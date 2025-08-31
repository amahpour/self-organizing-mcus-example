/**
 * @file hal_sim.c
 * @brief HAL implementation for Linux/macOS simulation platform
 *
 * This file implements the Hardware Abstraction Layer for running the
 * distributed system simulation on desktop operating systems. It uses
 * POSIX APIs for timing, standard library for random numbers, and
 * stdout for logging.
 *
 * Platform Features:
 * - High-resolution monotonic timing via clock_gettime()
 * - Thread-safe random number generation
 * - Console logging with printf()
 * - Cooperative multitasking via short sleeps
 */

#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include "../../core/hal.h"

/** Baseline timestamp for relative millisecond calculations */
static uint32_t g_start_time_ms = 0;

/**
 * @brief Initialize simulation HAL subsystem
 *
 * Sets up timing baseline and seeds the random number generator.
 * This implementation uses CLOCK_MONOTONIC for reliable timing
 * and current time for random seed.
 */
void hal_init(void) {
    // Establish timing baseline using high-resolution monotonic clock
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    g_start_time_ms = (uint32_t) (ts.tv_sec * 1000 + ts.tv_nsec / 1000000);

    // Seed random number generator with current time
    srand((unsigned) time(NULL));
}

/**
 * @brief Get milliseconds elapsed since hal_init() was called
 *
 * Uses CLOCK_MONOTONIC to provide a stable time reference that
 * isn't affected by system clock adjustments. Resolution is
 * typically 1ms or better on modern systems.
 *
 * @return Milliseconds since initialization
 */
uint32_t hal_millis(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    uint32_t now = (uint32_t) (ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
    return now - g_start_time_ms;
}

/**
 * @brief Block execution for specified milliseconds
 *
 * Uses usleep() to provide millisecond-granularity delays.
 * This is suitable for simulation timing but may not be
 * perfectly accurate due to OS scheduling.
 *
 * @param ms Number of milliseconds to delay
 */
void hal_delay(uint32_t ms) {
    usleep((useconds_t) (ms * 1000));
}

/**
 * @brief Yield CPU to other processes/threads
 *
 * Provides a short 1ms delay to allow other threads in the
 * simulation to make progress. This prevents busy-waiting
 * from consuming 100% CPU during polling loops.
 */
void hal_yield(void) {
    usleep(1000);  // 1ms yield for cooperative multitasking
}

/**
 * @brief Generate 32-bit pseudo-random number
 *
 * Combines two 16-bit rand() calls to create a 32-bit value
 * with reasonable distribution. Not cryptographically secure,
 * but sufficient for coordinator election tie-breaking.
 *
 * @return 32-bit pseudo-random value
 */
uint32_t hal_random32(void) {
    uint32_t high = (uint32_t) rand();
    uint32_t low = (uint32_t) rand();
    return (high << 16) ^ low;
}

/**
 * @brief Log message to standard output
 *
 * Outputs messages to stdout with automatic newline.
 * In a real simulation, this might be enhanced with
 * timestamps, node IDs, or log levels.
 *
 * @param msg Null-terminated message string
 */
void hal_log(const char* msg) {
    printf("%s\n", msg);
}
