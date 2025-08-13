#ifndef BUS_H
#define BUS_H

#include <stdint.h>
#include <stddef.h>
#include <pthread.h>
#include "proto.h"

#ifdef __cplusplus
extern "C" {
#endif

// The bus delivers every broadcast frame to all subscribers except, optionally, the sender.
// Implementation uses per-subscriber ring buffers with mutex/cond for simplicity.

// Opaque handle for subscriber
typedef struct BusSubscriber BusSubscriber;

// Initialize bus with a fixed maximum number of subscribers
int bus_init(size_t max_subscribers);
void bus_shutdown(void);

// Register a subscriber; returns handle or NULL on failure
BusSubscriber* bus_subscribe(uint8_t node_index);

// Broadcast a frame to all subscribers
int bus_broadcast(const Frame* frame);

// Receive next frame for this subscriber; timeout_ms = 0 for non-blocking, >0 for timed wait, (uint32_t)-1 for infinite
int bus_recv(BusSubscriber* sub, Frame* out, uint32_t timeout_ms);

#ifdef __cplusplus
}
#endif

#endif // BUS_H
