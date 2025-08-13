#ifndef NODE_H
#define NODE_H
#include <stdint.h>
#include "bus.h"
#include "proto.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    NODE_SEEKING = 0,
    NODE_COORDINATOR = 1,
    NODE_MEMBER = 2
} NodeRole;

typedef struct {
    Bus* bus;
    uint8_t instance_index;
    NodeRole role;
    uint8_t assigned_id;
    uint32_t random_nonce;
    uint8_t next_assign_id;
    uint32_t dedup[32];
    uint8_t seen_count;
    uint32_t join_nonce;
    uint32_t last_join_ms;
} Node;

void node_init(Node* n, Bus* bus, uint8_t instance_index);
void node_begin(Node* n);
void node_service(Node* n);

#ifdef __cplusplus
}
#endif
#endif
