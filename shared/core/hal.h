/**
 * @file hal.h
 * @brief Hardware Abstraction Layer for cross-platform compatibility
 *
 * This header defines a minimal HAL interface that abstracts platform-specific
 * functionality like timing, random number generation, and logging. Each target
 * platform provides its own implementation of these functions.
 *
 * Design Goals:
 * - Keep interface minimal to reduce porting effort
 * - Provide essential services needed by the node state machine
 * - Enable zero-ifdef core business logic
 * - Support both simulation and embedded targets
 */

#ifndef HAL_H
#define HAL_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize the hardware abstraction layer
 *
 * This function performs any platform-specific initialization required
 * for timing, random number generation, and logging subsystems.
 * Call this once during system startup before using other HAL functions.
 *
 * Platform Examples:
 * - Simulation: Initialize random seed, timing baseline
 * - Arduino: Initialize random seed from analog pin noise
 * - ESP32: Initialize high-resolution timer, entropy source
 */
void hal_init(void);

/**
 * @brief Get milliseconds elapsed since system start
 *
 * Returns a monotonic millisecond counter that starts at 0 when the
 * system boots. Used for timeouts, delays, and scheduling in the
 * node state machine.
 *
 * @return Milliseconds since system start (wraps after ~49 days)
 *
 * Platform Examples:
 * - Simulation: clock_gettime() relative to process start
 * - Arduino: millis() function
 * - ESP32: esp_timer_get_time() / 1000
 */
uint32_t hal_millis(void);

/**
 * @brief Block for the specified number of milliseconds
 *
 * This function blocks the calling thread/task for at least the specified
 * duration. Used for startup jitter and coordinator election timing.
 *
 * @param ms Number of milliseconds to delay
 *
 * Platform Examples:
 * - Simulation: usleep(ms * 1000)
 * - Arduino: delay(ms)
 * - ESP32: vTaskDelay(ms / portTICK_PERIOD_MS)
 */
void hal_delay(uint32_t ms);

/**
 * @brief Yield CPU to other tasks/threads (cooperative multitasking)
 *
 * This function allows other tasks to run during polling loops.
 * On single-threaded systems, this may be a no-op or a minimal delay.
 *
 * Platform Examples:
 * - Simulation: usleep(1000) for 1ms yield
 * - Arduino: yield() or delayMicroseconds(100)
 * - ESP32: taskYIELD() or vTaskDelay(1)
 */
void hal_yield(void);

/**
 * @brief Generate a 32-bit random number
 *
 * Returns a pseudo-random 32-bit value used for coordinator election
 * tie-breaking and JOIN request nonces. Quality doesn't need to be
 * cryptographically secure, but should have reasonable distribution.
 *
 * @return 32-bit pseudo-random value
 *
 * Platform Examples:
 * - Simulation: Combined rand() calls
 * - Arduino: Combined random() calls
 * - ESP32: esp_random()
 */
uint32_t hal_random32(void);

/**
 * @brief Log a message to the platform's output system
 *
 * This function outputs diagnostic messages for debugging and monitoring.
 * The message should be null-terminated and reasonably short (< 100 chars).
 *
 * @param msg Null-terminated message string to log
 *
 * Platform Examples:
 * - Simulation: printf() to stdout
 * - Arduino: Serial.println()
 * - ESP32: ESP_LOGI() or printf()
 */
void hal_log(const char* msg);

#ifdef __cplusplus
}
#endif

#endif  // HAL_H
