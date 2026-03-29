#include <Arduino.h>

#include "../../core/hal.h"

void hal_init(void) {
    randomSeed(analogRead(A0));
}

uint32_t hal_millis(void) {
    return millis();
}

void hal_delay(uint32_t ms) {
    delay(ms);
}

void hal_yield(void) {
    yield();
}

uint32_t hal_random32(void) {
    uint32_t a = (uint32_t) random(0, 65536);
    uint32_t b = (uint32_t) random(0, 65536);
    return (a << 16) | b;
}

void hal_log(const char* msg) {
    Serial.println(msg);
}

void hal_gpio_set_output(uint8_t pin) {
    pinMode(pin, OUTPUT);
}

void hal_gpio_write(uint8_t pin, uint8_t value) {
    digitalWrite(pin, value ? HIGH : LOW);
}

