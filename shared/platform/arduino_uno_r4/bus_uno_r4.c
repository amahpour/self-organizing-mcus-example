/**
 * @file bus_uno_r4.c
 * @brief Arduino UNO R4 WiFi hardware serial bus implementation
 *
 * Uses hardware Serial1 (pins 0 RX, 1 TX) instead of SoftwareSerial.
 * This avoids all the SoftwareSerial issues on the Renesas RA4M1 architecture.
 */

#include <Arduino.h>
#include <stdlib.h>

#include "../../core/bus_interface.h"
#include "../../core/hal.h"

struct Bus {
    HardwareSerial* serial;
};

int bus_global_init(uint8_t max_nodes) {
    (void) max_nodes;  // Not needed for hardware serial
    return 0;
}

void bus_global_shutdown(void) {
    // Nothing to do
}

int bus_create(Bus** bus, uint8_t node_index, uint8_t rx_pin, uint8_t tx_pin) {
    (void) node_index;  // Not used for UART
    (void) rx_pin;      // Hardware serial pins are fixed (0 RX, 1 TX)
    (void) tx_pin;      // Hardware serial pins are fixed (0 RX, 1 TX)

    Bus* b = (Bus*) malloc(sizeof(Bus));
    if (!b)
        return -1;

    // Use Serial1 (pins 0 RX, 1 TX) on Arduino UNO R4 WiFi
    b->serial = &Serial1;
    b->serial->begin(19200);
    
    *bus = b;
    return 0;
}

void bus_destroy(Bus* bus) {
    if (bus) {
        if (bus->serial) {
            bus->serial->end();
        }
        free(bus);
    }
}

int bus_send(Bus* bus, const Frame* frame) {
    if (!bus || !bus->serial || !frame)
        return -1;

    Frame f = *frame;
    proto_finalize(&f);

    const uint8_t* raw = (const uint8_t*) &f;
    size_t len = 5 + f.payload_len + 1;  // sof + header + payload + checksum

    return bus->serial->write(raw, len) == (int) len ? 1 : 0;
}

static int read_byte(HardwareSerial* serial, uint8_t* byte, uint16_t timeout_ms) {
    uint32_t start = hal_millis();
    while ((hal_millis() - start) < timeout_ms) {
        if (serial->available()) {
            *byte = (uint8_t) serial->read();
            return 1;
        }
        hal_yield();
    }
    return 0;
}

int bus_recv(Bus* bus, Frame* frame, uint16_t timeout_ms) {
    if (!bus || !bus->serial || !frame)
        return -1;

    uint32_t start = hal_millis();

    // Find start-of-frame
    while ((hal_millis() - start) < timeout_ms) {
        if (bus->serial->available()) {
            uint8_t b = (uint8_t) bus->serial->read();
            if (b == SOF) {
                frame->sof = b;

                // Read fixed header
                if (!read_byte(bus->serial, &frame->type, timeout_ms))
                    return 0;
                if (!read_byte(bus->serial, &frame->source, timeout_ms))
                    return 0;
                if (!read_byte(bus->serial, &frame->payload_len, timeout_ms))
                    return 0;

                if (frame->payload_len > MAX_PAYLOAD_SIZE)
                    return 0;

                // Read payload
                for (uint8_t i = 0; i < frame->payload_len; ++i) {
                    if (!read_byte(bus->serial, &frame->payload[i], timeout_ms))
                        return 0;
                }

                // Read checksum
                if (!read_byte(bus->serial, &frame->checksum, timeout_ms))
                    return 0;

                return proto_is_valid(frame) ? 1 : 0;
            }
        }
        hal_yield();
    }

    return 0;  // Timeout
}
