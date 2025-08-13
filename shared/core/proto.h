#ifndef PROTO_H
#define PROTO_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Protocol definitions - completely platform-agnostic

#define SOF 0xAA
#define MAX_PAYLOAD_SIZE 8

typedef enum {
    MSG_HELLO = 1,
    MSG_CLAIM = 2,
    MSG_JOIN = 3,
    MSG_ASSIGN = 4,
    MSG_HEARTBEAT = 5
} MessageType;

typedef struct {
    uint8_t sof;
    uint8_t type;
    uint8_t source;
    uint8_t payload_len;
    uint8_t payload[MAX_PAYLOAD_SIZE];
    uint8_t checksum;
} Frame;

// Protocol utilities
uint8_t proto_compute_checksum(const Frame* f);
void proto_finalize(Frame* f);
int proto_is_valid(const Frame* f);

// Utility functions for payload packing
void u32_to_bytes(uint32_t v, uint8_t out[4]);
uint32_t bytes_to_u32(const uint8_t in[4]);

#ifdef __cplusplus
}
#endif

#endif // PROTO_H
