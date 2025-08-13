#ifndef SHARED_PROTO_H
#define SHARED_PROTO_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Start-of-frame byte for basic framing
#define SOF 0xAA

// Maximum payload size kept tiny for didactic clarity
#define MAX_PAYLOAD_SIZE 8

typedef enum {
    MSG_HELLO = 1,
    MSG_CLAIM = 2,
    MSG_JOIN = 3,
    MSG_ASSIGN = 4,
    MSG_HEARTBEAT = 5
} MessageType;

// Frame layout: SOF | type | source | payload_len | payload | checksum
// checksum is an 8-bit XOR over [type..payload]
typedef struct {
    uint8_t sof;         // always SOF
    uint8_t type;        // MessageType
    uint8_t source;      // 0 for unknown/unassigned, >0 for assigned IDs
    uint8_t payload_len; // 0..MAX_PAYLOAD_SIZE
    uint8_t payload[MAX_PAYLOAD_SIZE];
    uint8_t checksum;    // XOR of type, source, payload_len, payload bytes
} Frame;

static inline uint8_t proto_compute_checksum(const Frame* f) {
    uint8_t x = 0;
    x ^= f->type;
    x ^= f->source;
    x ^= f->payload_len;
    for (uint8_t i = 0; i < f->payload_len; ++i) {
        x ^= f->payload[i];
    }
    return x;
}

static inline void proto_finalize(Frame* f) {
    f->sof = SOF;
    if (f->payload_len > MAX_PAYLOAD_SIZE) f->payload_len = MAX_PAYLOAD_SIZE;
    f->checksum = proto_compute_checksum(f);
}

static inline int proto_is_valid(const Frame* f) {
    if (f->sof != SOF) return 0;
    if (f->payload_len > MAX_PAYLOAD_SIZE) return 0;
    return proto_compute_checksum(f) == f->checksum;
}

#ifdef __cplusplus
}
#endif

#endif // SHARED_PROTO_H
