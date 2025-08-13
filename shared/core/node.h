#ifndef NODE_H
#define NODE_H

#include <stdint.h>
#include "proto.h"
#include "bus_interface.h"

#ifdef __cplusplus
extern "C" {
#endif

// Node state machine - completely platform-agnostic business logic

typedef enum {
    NODE_SEEKING = 0,
    NODE_COORDINATOR = 1,
    NODE_MEMBER = 2
} NodeRole;

#define NODE_MAX_DEDUP 32

typedef struct {
    Bus* bus;
    uint8_t instance_index;
    NodeRole role;
    uint8_t assigned_id;
    uint32_t random_nonce;
    uint8_t next_assign_id;
    
    // Deduplication state
    uint32_t seen_join_nonce[NODE_MAX_DEDUP];
    uint8_t seen_count;
    
    // Member state
    uint32_t join_nonce;
    uint32_t last_join_ms;
} Node;

// Node lifecycle
void node_init(Node* n, Bus* bus, uint8_t instance_index);
void node_begin(Node* n);
void node_service(Node* n);

#ifdef __cplusplus
}
#endif

#endif // NODE_H
