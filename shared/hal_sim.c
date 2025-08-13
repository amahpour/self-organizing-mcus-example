#define _POSIX_C_SOURCE 200809L
#include "hal.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

static uint64_t start_ms;

void hal_init(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    start_ms = (uint64_t)ts.tv_sec * 1000ULL + ts.tv_nsec / 1000000ULL;
    srand((unsigned)time(NULL));
}

uint32_t hal_millis(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    uint64_t now = (uint64_t)ts.tv_sec * 1000ULL + ts.tv_nsec / 1000000ULL;
    return (uint32_t)(now - start_ms);
}

void hal_delay(unsigned long ms) {
    usleep(ms * 1000);
}

void hal_yield(void) {
    // no-op in simulation
}

uint32_t hal_random32(void) {
    return ((uint32_t)rand() << 16) ^ (uint32_t)rand();
}

void hal_log(const char* msg) {
    printf("%s\n", msg);
    fflush(stdout);
}
