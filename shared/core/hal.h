#ifndef HAL_H
#define HAL_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Hardware Abstraction Layer
// Platform-agnostic interface for timing, randomness, and logging

void hal_init(void);
uint32_t hal_millis(void);
void hal_delay(uint32_t ms);
void hal_yield(void);
uint32_t hal_random32(void);
void hal_log(const char* msg);

#ifdef __cplusplus
}
#endif

#endif // HAL_H
