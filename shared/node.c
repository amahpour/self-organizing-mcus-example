#include "node.h"
#include "hal.h"
#include <string.h>
#include <stdio.h>

static void u32_to_bytes(uint32_t v, uint8_t out[4]) {
    out[0] = (uint8_t)(v >> 24);
    out[1] = (uint8_t)(v >> 16);
    out[2] = (uint8_t)(v >> 8);
    out[3] = (uint8_t)(v);
}

static uint32_t bytes_to_u32(const uint8_t in[4]) {
    return ((uint32_t)in[0] << 24) | ((uint32_t)in[1] << 16) |
           ((uint32_t)in[2] << 8) | ((uint32_t)in[3]);
}

static int seen(Node* n, uint32_t nonce) {
    for (uint8_t i = 0; i < n->seen_count; ++i) if (n->dedup[i] == nonce) return 1;
    if (n->seen_count < 32) n->dedup[n->seen_count++] = nonce;
    else n->dedup[n->seen_count % 32] = nonce;
    return 0;
}

void node_init(Node* n, Bus* bus, uint8_t instance_index) {
    memset(n, 0, sizeof(*n));
    n->bus = bus;
    n->instance_index = instance_index;
}

void node_begin(Node* n) {
    n->role = NODE_SEEKING;
    n->assigned_id = 0;
    n->random_nonce = hal_random32();
    n->seen_count = 0;
    n->last_join_ms = 0;

    hal_delay((unsigned long)n->instance_index * 150);

    Frame in;
    int heard = 0;
    uint32_t endAt = hal_millis() + 200;
    while (hal_millis() < endAt) {
        if (bus_recv(n->bus, &in, 20) && in.type == MSG_CLAIM) { heard = 1; break; }
        hal_yield();
    }

    if (!heard) {
        uint8_t p[4]; u32_to_bytes(n->random_nonce, p);
        Frame claim = { SOF, MSG_CLAIM, 0, 4, {0}, 0 };
        memcpy(claim.payload, p, 4);
        proto_finalize(&claim);
        bus_send(n->bus, &claim);
        char buf[64]; snprintf(buf, sizeof(buf), "CLAIM nonce=%u", n->random_nonce);
        hal_log(buf);

        int lost = 0;
        endAt = hal_millis() + 150;
        while (hal_millis() < endAt) {
            if (bus_recv(n->bus, &in, 20) && in.type == MSG_CLAIM && in.payload_len >= 4) {
                if (bytes_to_u32(in.payload) > n->random_nonce) { lost = 1; break; }
            }
            hal_yield();
        }
        if (!lost) {
            n->role = NODE_COORDINATOR; n->assigned_id = 1; n->next_assign_id = 2;
            hal_log("→ COORDINATOR (ID=1)");
        }
    }

    if (n->role == NODE_SEEKING) {
        Frame hello = { SOF, MSG_HELLO, 0, 0, {0}, 0 }; proto_finalize(&hello); bus_send(n->bus, &hello);
        hal_log("HELLO");

        n->join_nonce = hal_random32();
        uint8_t p[4]; u32_to_bytes(n->join_nonce, p);
        Frame join = { SOF, MSG_JOIN, 0, 4, {0}, 0 }; memcpy(join.payload, p, 4); proto_finalize(&join);
        bus_send(n->bus, &join);
        n->last_join_ms = hal_millis();
        char buf[64]; snprintf(buf, sizeof(buf), "JOIN (nonce=%u)", n->join_nonce); hal_log(buf);
    }
}

void node_service(Node* n) {
    Frame in;
    if (bus_recv(n->bus, &in, 50)) {
        if (!proto_is_valid(&in)) return;
        if (n->role == NODE_COORDINATOR) {
            if (in.type == MSG_JOIN && in.payload_len >= 4) {
                uint32_t nonce = bytes_to_u32(in.payload);
                if (seen(n, nonce)) return;
                uint8_t id = n->next_assign_id++;
                Frame assign = { SOF, MSG_ASSIGN, 1, 5, {0}, 0 };
                assign.payload[0] = id;
                memcpy(&assign.payload[1], in.payload, 4);
                proto_finalize(&assign);
                bus_send(n->bus, &assign);
                char buf[64]; snprintf(buf, sizeof(buf), "ASSIGN → id=%u", id); hal_log(buf);
            }
        } else if (n->role == NODE_SEEKING) {
            if (in.type == MSG_ASSIGN && in.payload_len >= 5) {
                uint8_t id = in.payload[0];
                uint32_t echoed = bytes_to_u32(&in.payload[1]);
                if (echoed == n->join_nonce) {
                    n->assigned_id = id; n->role = NODE_MEMBER;
                    char buf[64]; snprintf(buf, sizeof(buf), "ASSIGN received → MEMBER (ID=%u)", n->assigned_id); hal_log(buf);
                }
            }
        }
    }
    if (n->role == NODE_SEEKING && (hal_millis() - n->last_join_ms) >= 250) {
        uint8_t p[4]; u32_to_bytes(n->join_nonce, p);
        Frame join = { SOF, MSG_JOIN, 0, 4, {0}, 0 }; memcpy(join.payload, p, 4); proto_finalize(&join);
        bus_send(n->bus, &join);
        n->last_join_ms = hal_millis();
    }
}
