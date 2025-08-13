#include "hal.h"
#include <Arduino.h>

void hal_init(void) {}

uint32_t hal_millis(void) { return millis(); }

void hal_delay(unsigned long ms) { delay(ms); }

void hal_yield(void) { yield(); }

uint32_t hal_random32(void) {
    return ((uint32_t)random(0xFFFF) << 16) ^ (uint32_t)random(0xFFFF);
}

void hal_log(const char* msg) {
    Serial.println(msg);
}
