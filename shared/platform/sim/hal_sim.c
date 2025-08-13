#include "../../core/hal.h"
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

static uint32_t g_start_time_ms = 0;

void hal_init(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    g_start_time_ms = (uint32_t)(ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
    srand((unsigned)time(NULL));
}

uint32_t hal_millis(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    uint32_t now = (uint32_t)(ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
    return now - g_start_time_ms;
}

void hal_delay(uint32_t ms) {
    usleep((useconds_t)(ms * 1000));
}

void hal_yield(void) {
    usleep(1000); // 1ms yield for cooperative multitasking
}

uint32_t hal_random32(void) {
    uint32_t a = (uint32_t)rand();
    uint32_t b = (uint32_t)rand();
    return (a << 16) ^ b;
}

void hal_log(const char* msg) {
    printf("%s\n", msg);
}
