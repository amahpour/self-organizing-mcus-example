#include "node.h"
#include "hal.h"
#include <string.h>
#include <stdio.h>

// Helper functions
static int coordinator_seen_nonce(Node* n, uint32_t nonce) {
    for (uint8_t i = 0; i < n->seen_count; ++i) {
        if (n->seen_join_nonce[i] == nonce) return 1;
    }
    if (n->seen_count < NODE_MAX_DEDUP) {
        n->seen_join_nonce[n->seen_count++] = nonce;
    } else {
        // Ring buffer overwrite when full
        n->seen_join_nonce[n->seen_count % NODE_MAX_DEDUP] = nonce;
        n->seen_count = (uint8_t)((n->seen_count + 1) % NODE_MAX_DEDUP);
    }
    return 0;
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

void node_init(Node* n, Bus* bus, uint8_t instance_index) {
    memset(n, 0, sizeof(*n));
    n->bus = bus;
    n->instance_index = instance_index;
}

void node_begin(Node* n) {
    // Startup jitter based on instance index
    hal_delay((uint32_t)n->instance_index * 150);
    
    // Initialize state
    n->role = NODE_SEEKING;
    n->assigned_id = 0;
    n->random_nonce = hal_random32();
    n->seen_count = 0;
    n->last_join_ms = 0;
    
    // Listen for existing CLAIM messages
    Frame in;
    int heard_claim = 0;
    uint32_t listen_end = hal_millis() + 200;
    
    while (hal_millis() < listen_end) {
        if (bus_recv(n->bus, &in, 50) && proto_is_valid(&in) && in.type == MSG_CLAIM) {
            heard_claim = 1;
            break;
        }
        hal_yield();
    }
    
    if (!heard_claim) {
        // Claim coordinator role
        uint8_t payload[4];
        u32_to_bytes(n->random_nonce, payload);
        Frame claim;
        make_frame(&claim, MSG_CLAIM, 0, payload, 4);
        bus_send(n->bus, &claim);
        
        char msg[64];
        snprintf(msg, sizeof(msg), "Node[%u] CLAIM nonce=%u", n->instance_index, n->random_nonce);
        hal_log(msg);
        
        // Conflict detection window
        int lost = 0;
        uint32_t conflict_end = hal_millis() + 150;
        
        while (hal_millis() < conflict_end) {
            if (bus_recv(n->bus, &in, 50) && proto_is_valid(&in) && 
                in.type == MSG_CLAIM && in.payload_len >= 4) {
                uint32_t other_nonce = bytes_to_u32(in.payload);
                if (other_nonce > n->random_nonce) {
                    lost = 1;
                    break;
                }
            }
            hal_yield();
        }
        
        if (!lost) {
            n->role = NODE_COORDINATOR;
            n->assigned_id = 1;
            n->next_assign_id = 2;
            
            snprintf(msg, sizeof(msg), "Node[%u] → COORDINATOR (ID=1)", n->instance_index);
            hal_log(msg);
        }
    }
    
    if (n->role == NODE_SEEKING) {
        // Join as member
        Frame hello;
        make_frame(&hello, MSG_HELLO, 0, NULL, 0);
        bus_send(n->bus, &hello);
        hal_log("HELLO");
        
        n->join_nonce = hal_random32();
        uint8_t payload[4];
        u32_to_bytes(n->join_nonce, payload);
        Frame join;
        make_frame(&join, MSG_JOIN, 0, payload, 4);
        bus_send(n->bus, &join);
        n->last_join_ms = hal_millis();
        
        char msg[64];
        snprintf(msg, sizeof(msg), "JOIN (nonce=%u)", n->join_nonce);
        hal_log(msg);
    }
}

void node_service(Node* n) {
    Frame in;
    
    // Process incoming messages
    if (bus_recv(n->bus, &in, 50) && proto_is_valid(&in)) {
        if (n->role == NODE_COORDINATOR) {
            if (in.type == MSG_JOIN && in.payload_len >= 4) {
                uint32_t nonce = bytes_to_u32(in.payload);
                if (coordinator_seen_nonce(n, nonce)) {
                    return; // Already assigned for this nonce
                }
                
                // Assign next ID
                uint8_t id = n->next_assign_id++;
                uint8_t payload[5];
                payload[0] = id;
                memcpy(&payload[1], in.payload, 4);
                Frame assign;
                make_frame(&assign, MSG_ASSIGN, 1, payload, 5);
                bus_send(n->bus, &assign);
                
                char msg[32];
                snprintf(msg, sizeof(msg), "ASSIGN → id=%u", id);
                hal_log(msg);
            }
        } else if (n->role == NODE_SEEKING) {
            if (in.type == MSG_ASSIGN && in.payload_len >= 5) {
                uint8_t assigned = in.payload[0];
                uint32_t echoed = bytes_to_u32(&in.payload[1]);
                if (echoed == n->join_nonce) {
                    n->assigned_id = assigned;
                    n->role = NODE_MEMBER;
                    
                    char msg[64];
                    snprintf(msg, sizeof(msg), "ASSIGN received → MEMBER (ID=%u)", n->assigned_id);
                    hal_log(msg);
                }
            }
        }
    }
    
    // Re-send JOIN if still seeking and timeout elapsed
    if (n->role == NODE_SEEKING && (hal_millis() - n->last_join_ms) >= 250) {
        uint8_t payload[4];
        u32_to_bytes(n->join_nonce, payload);
        Frame join;
        make_frame(&join, MSG_JOIN, 0, payload, 4);
        bus_send(n->bus, &join);
        n->last_join_ms = hal_millis();
    }
}
