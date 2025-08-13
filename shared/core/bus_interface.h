#ifndef BUS_INTERFACE_H
#define BUS_INTERFACE_H

#include <stdint.h>
#include "proto.h"

#ifdef __cplusplus
extern "C" {
#endif

// Bus Interface - Platform-agnostic communication abstraction
// Implementations handle the actual transport (UART, in-process queues, etc.)

// Opaque bus handle - platform implementations define the actual struct
typedef struct Bus Bus;

// Global bus initialization (for platforms that need it, like sim)
int bus_global_init(uint8_t max_nodes);
void bus_global_shutdown(void);

// Per-bus operations
int bus_create(Bus** bus, uint8_t node_index, uint8_t rx_pin, uint8_t tx_pin);
void bus_destroy(Bus* bus);
int bus_send(Bus* bus, const Frame* frame);
int bus_recv(Bus* bus, Frame* frame, uint16_t timeout_ms);

#ifdef __cplusplus
}
#endif

#endif // BUS_INTERFACE_H
