#ifndef BUS_H
#define BUS_H
#include "proto.h"
#include <stdint.h>
#ifdef ARDUINO
#include <SoftwareSerial.h>
typedef struct Bus {
    SoftwareSerial serial;
} Bus;
#else
typedef struct Bus {
    uint8_t index;
} Bus;
#endif
#ifdef __cplusplus
extern "C" {
#endif
#ifndef ARDUINO
int bus_global_init(uint8_t max_nodes);
void bus_global_shutdown(void);
#endif
int bus_init(Bus* bus, uint8_t node_index, uint8_t rx_pin, uint8_t tx_pin);
int bus_send(Bus* bus, const Frame* f);
int bus_recv(Bus* bus, Frame* out, uint16_t timeout_ms);
#ifdef __cplusplus
}
#endif
#endif
