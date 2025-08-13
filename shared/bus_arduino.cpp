#include "bus.h"
#include "hal.h"
#include <Arduino.h>
#include <string.h>

int bus_global_init(uint8_t max_nodes) { (void)max_nodes; return 0; }
void bus_global_shutdown(void) {}

int bus_init(Bus* bus, uint8_t node_index, uint8_t rx_pin, uint8_t tx_pin) {
    (void)node_index;
    new (&bus->serial) SoftwareSerial(rx_pin, tx_pin, true);
    bus->serial.begin(19200);
    return 0;
}

int bus_send(Bus* bus, const Frame* f) {
    Frame c = *f;
    proto_finalize(&c);
    const uint8_t* raw = (const uint8_t*)&c;
    size_t len = 5 + c.payload_len + 1;
    return bus->serial.write(raw, len) == (int)len ? 1 : 0;
}

int bus_recv(Bus* bus, Frame* out, uint16_t timeout_ms) {
    uint32_t start = hal_millis();
    while ((hal_millis() - start) < timeout_ms) {
        if (bus->serial.available()) {
            uint8_t b = (uint8_t)bus->serial.read();
            if (b == SOF) {
                out->sof = b;
                if (!bus->serial.readBytes(&out->type, 1)) return 0;
                if (!bus->serial.readBytes(&out->source, 1)) return 0;
                if (!bus->serial.readBytes(&out->payload_len, 1)) return 0;
                if (out->payload_len > MAX_PAYLOAD_SIZE) return 0;
                for (uint8_t i=0;i<out->payload_len;i++) {
                    if (!bus->serial.readBytes(&out->payload[i],1)) return 0;
                }
                if (!bus->serial.readBytes(&out->checksum,1)) return 0;
                return proto_is_valid(out);
            }
        }
        hal_yield();
    }
    return 0;
}
