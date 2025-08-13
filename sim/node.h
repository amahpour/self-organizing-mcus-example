#ifndef NODE_H
#define NODE_H

#include <stdint.h>
#include <pthread.h>
#include "proto.h"
#include "bus.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    NODE_SEEKING = 0,
    NODE_COORDINATOR = 1,
    NODE_MEMBER = 2
} NodeRole;

#define NODE_MAX_DEDUP 64

typedef struct {
    uint8_t instance_index;    // index unique per process instance
    uint8_t assigned_id;       // 0 until assigned; coordinator uses 1
    uint32_t random_nonce;     // for tie-break on CLAIM
    NodeRole role;

    BusSubscriber* sub;

    // coordinator state
    uint8_t next_assign_id;    // next ID to assign (starts at 2 when coordinator)
    uint32_t seen_join_nonce[NODE_MAX_DEDUP];
    uint8_t seen_count;

    // thread
    pthread_t thread;
    int running;

    // log prefix buffer
    char name[32];
} Node;

// Create and start a node thread
int node_start(Node* n, uint8_t instance_index);

// Request node shutdown and join thread
void node_stop(Node* n);

#ifdef __cplusplus
}
#endif

#endif // NODE_H
