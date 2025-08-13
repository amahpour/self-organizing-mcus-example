#pragma once
#include <Arduino.h>
extern "C" {
#include "../shared/proto.h"
}

inline void u32_to_bytes(uint32_t v, uint8_t out[4]) {
  out[0] = (uint8_t)(v >> 24);
  out[1] = (uint8_t)(v >> 16);
  out[2] = (uint8_t)(v >> 8);
  out[3] = (uint8_t)(v);
}

inline uint32_t bytes_to_u32(const uint8_t in[4]) {
  return ((uint32_t)in[0] << 24) | ((uint32_t)in[1] << 16) | ((uint32_t)in[2] << 8) | ((uint32_t)in[3]);
}
