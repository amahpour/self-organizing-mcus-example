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
    b->serial->begin(9600);  // Match ping-pong baud rate
    
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

void bus_set_baud(Bus* bus, uint32_t baud) {
    if (bus && bus->serial) {
        bus->serial->end();
        bus->serial->begin(baud);
    }
}

int bus_send(Bus* bus, const Frame* frame) {
    if (!bus || !bus->serial || !frame)
        return -1;

    Frame f = *frame;
    proto_finalize(&f);

    // Manually serialize frame to avoid struct padding issues
    uint8_t buffer[5 + MAX_PAYLOAD_SIZE + 1]; // max possible frame size
    buffer[0] = f.sof;
    buffer[1] = f.type;
    buffer[2] = f.source;
    buffer[3] = f.payload_len;
    
    // Copy only the used payload bytes
    for (uint8_t i = 0; i < f.payload_len; i++) {
        buffer[4 + i] = f.payload[i];
    }
    
    buffer[4 + f.payload_len] = f.checksum;
    
    size_t len = 5 + f.payload_len;  // sof + type + source + len + payload + checksum

    // Debug output
    Serial.print("DEBUG: [R4] Sending frame: ");
    for (size_t i = 0; i < len; i++) {
        Serial.print("0x");
        Serial.print(buffer[i], HEX);
        Serial.print(" ");
    }
    Serial.println();

    int result = bus->serial->write(buffer, len) == (int) len ? 1 : 0;
    Serial.println("DEBUG: [R4] Send result: " + String(result));
    return result;
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
            Serial.println("DEBUG: [R4] Received byte: 0x" + String(b, HEX));
            if (b == SOF) {
                Serial.println("DEBUG: [R4] Found SOF, reading frame...");
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

                Serial.println("DEBUG: [R4] Frame complete - type=" + String(frame->type) + " source=" + String(frame->source));
                int valid = proto_is_valid(frame);
                Serial.println("DEBUG: [R4] Frame valid: " + String(valid));
                return valid ? 1 : 0;
            }
        }
        hal_yield();
    }

    return 0;  // Timeout
}
