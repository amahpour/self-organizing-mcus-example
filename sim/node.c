#include "node.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

static uint32_t rand32(void) {
    uint32_t a = (uint32_t)rand();
    uint32_t b = (uint32_t)rand();
    return (a << 16) ^ b;
}

static void make_frame(Frame* f, MessageType type, uint8_t source, const void* payload, uint8_t len) {
    memset(f, 0, sizeof(*f));
    f->type = (uint8_t)type;
    f->source = source;
    f->payload_len = len > MAX_PAYLOAD_SIZE ? MAX_PAYLOAD_SIZE : len;
    if (payload && f->payload_len) {
        memcpy(f->payload, payload, f->payload_len);
    }
    proto_finalize(f);
}

static void u32_to_bytes(uint32_t v, uint8_t out[4]) {
    out[0] = (uint8_t)(v >> 24);
    out[1] = (uint8_t)(v >> 16);
    out[2] = (uint8_t)(v >> 8);
    out[3] = (uint8_t)(v);
}

static uint32_t bytes_to_u32(const uint8_t in[4]) {
    return ((uint32_t)in[0] << 24) | ((uint32_t)in[1] << 16) | ((uint32_t)in[2] << 8) | ((uint32_t)in[3]);
}

static int coordinator_seen_nonce(Node* n, uint32_t nonce) {
    for (uint8_t i = 0; i < n->seen_count; ++i) {
        if (n->seen_join_nonce[i] == nonce) return 1;
    }
    if (n->seen_count < NODE_MAX_DEDUP) {
        n->seen_join_nonce[n->seen_count++] = nonce;
    } else {
        // simple ring overwrite when full
        n->seen_join_nonce[n->seen_count % NODE_MAX_DEDUP] = nonce;
        n->seen_count = (uint8_t)((n->seen_count + 1) % NODE_MAX_DEDUP);
    }
    return 0;
}

static void* node_thread_main(void* arg) {
    Node* n = (Node*)arg;

    // Small startup jitter to model staggered boots (150ms per index)
    usleep((useconds_t)(n->instance_index * 150000));

    // Initial seeking: listen briefly for CLAIM
    n->role = NODE_SEEKING;
    n->assigned_id = 0;
    n->random_nonce = rand32();
    n->seen_count = 0;

    // Listen window ~200ms for any CLAIM
    Frame in;
    int heard_claim = 0;
    uint32_t listen_left_ms = 200;
    while (listen_left_ms > 0) {
        int rc = bus_recv(n->sub, &in, 50);
        if (rc == 0 && proto_is_valid(&in) && in.type == MSG_CLAIM) {
            heard_claim = 1;
            break;
        }
        if (rc == 1) { // timeout
            if (listen_left_ms >= 50) listen_left_ms -= 50; else listen_left_ms = 0;
        }
    }

    if (!heard_claim) {
        // Claim coordinator with nonce
        Frame claim;
        uint8_t payload[4];
        uint32_t nonce = n->random_nonce;
        u32_to_bytes(nonce, payload);
        make_frame(&claim, MSG_CLAIM, 0 /*unknown*/, payload, 4);
        bus_broadcast(&claim);
        printf("%s CLAIM nonce=%u\n", n->name, nonce);

        // Short conflict window: if we hear a CLAIM with a higher nonce, we yield
        int lost = 0;
        uint32_t wait_ms = 150;
        while (wait_ms > 0) {
            int rc = bus_recv(n->sub, &in, 50);
            if (rc == 0 && proto_is_valid(&in) && in.type == MSG_CLAIM && in.payload_len >= 4) {
                uint32_t other = bytes_to_u32(in.payload);
                if (other > nonce) {
                    lost = 1;
                    break;
                }
            }
            if (rc == 1) {
                if (wait_ms >= 50) wait_ms -= 50; else wait_ms = 0;
            }
        }

        if (!lost) {
            // We become coordinator
            n->role = NODE_COORDINATOR;
            n->assigned_id = 1;
            n->next_assign_id = 2;
            printf("%s → COORDINATOR (ID=1)\n", n->name);
        }
    }

    if (n->role == NODE_SEEKING) {
        // Join as member: send HELLO and JOIN with a join_nonce, then periodically re-send JOIN until ASSIGN
        Frame hello;
        make_frame(&hello, MSG_HELLO, 0, NULL, 0);
        bus_broadcast(&hello);
        printf("%s HELLO\n", n->name);

        uint32_t join_nonce = rand32();
        uint8_t join_payload[4];
        u32_to_bytes(join_nonce, join_payload);

        Frame join;
        make_frame(&join, MSG_JOIN, 0, join_payload, 4);
        bus_broadcast(&join);
        printf("%s JOIN (request ID, nonce=%u)\n", n->name, join_nonce);

        // Await ASSIGN, re-sending JOIN every 250ms
        uint32_t wait_ms = 3000;
        uint32_t resend_ms = 250;
        while (wait_ms > 0 && n->assigned_id == 0) {
            int rc = bus_recv(n->sub, &in, 100);
            if (rc == 0 && proto_is_valid(&in) && in.type == MSG_ASSIGN && in.payload_len >= 5) {
                uint8_t assigned = in.payload[0];
                uint32_t echoed = bytes_to_u32(&in.payload[1]);
                if (echoed == join_nonce) {
                    n->assigned_id = assigned;
                    n->role = NODE_MEMBER;
                    printf("%s ASSIGN received → MEMBER (ID=%u)\n", n->name, n->assigned_id);
                    break;
                }
            }
            if (rc == 1) {
                // timeout slice
            }
            if (resend_ms <= 100) {
                // resend JOIN
                bus_broadcast(&join);
                resend_ms = 250;
            } else {
                resend_ms -= 100;
            }
            if (wait_ms >= 100) wait_ms -= 100; else wait_ms = 0;
        }

        if (n->assigned_id == 0) {
            printf("%s timeout waiting for ASSIGN\n", n->name);
        }
    }

    // Main service loop
    while (n->running) {
        int rc = bus_recv(n->sub, &in, 200);
        if (rc != 0) continue;
        if (!proto_is_valid(&in)) continue;

        if (n->role == NODE_COORDINATOR) {
            if (in.type == MSG_JOIN && in.payload_len >= 4) {
                uint32_t nonce = bytes_to_u32(in.payload);
                if (coordinator_seen_nonce(n, nonce)) {
                    // already assigned for this nonce
                    continue;
                }
                // Assign next ID, echoing the join_nonce so only the requester accepts
                uint8_t id = n->next_assign_id++;
                uint8_t payload[5];
                payload[0] = id;
                memcpy(&payload[1], in.payload, 4);
                Frame assign;
                make_frame(&assign, MSG_ASSIGN, 1 /*coordinator id*/, payload, 5);
                bus_broadcast(&assign);
                printf("%s ASSIGN → id=%u\n", n->name, id);
            }
        } else if (n->role == NODE_MEMBER) {
            // For now, members just observe
            (void)in; // placeholder
        } else {
            // still seeking: ignore
        }
    }

    return NULL;
}

int node_start(Node* n, uint8_t instance_index) {
    memset(n, 0, sizeof(*n));
    n->instance_index = instance_index;
    snprintf(n->name, sizeof(n->name), "Node[%u]", (unsigned)instance_index);
    n->sub = bus_subscribe(instance_index);
    if (!n->sub) return -1;
    n->running = 1;
    if (pthread_create(&n->thread, NULL, node_thread_main, n) != 0) {
        n->running = 0;
        return -1;
    }
    return 0;
}

void node_stop(Node* n) {
    if (!n->running) return;
    n->running = 0;
    pthread_join(n->thread, NULL);
}
