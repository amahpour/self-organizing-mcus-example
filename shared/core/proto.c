#include "proto.h"

uint8_t proto_compute_checksum(const Frame* f) {
    uint8_t x = 0;
    x ^= f->type;
    x ^= f->source;
    x ^= f->payload_len;
    for (uint8_t i = 0; i < f->payload_len; ++i) {
        x ^= f->payload[i];
    }
    return x;
}

void proto_finalize(Frame* f) {
    f->sof = SOF;
    if (f->payload_len > MAX_PAYLOAD_SIZE) {
        f->payload_len = MAX_PAYLOAD_SIZE;
    }
    f->checksum = proto_compute_checksum(f);
}

int proto_is_valid(const Frame* f) {
    if (f->sof != SOF) return 0;
    if (f->payload_len > MAX_PAYLOAD_SIZE) return 0;
    return proto_compute_checksum(f) == f->checksum;
}

void u32_to_bytes(uint32_t v, uint8_t out[4]) {
    out[0] = (uint8_t)(v >> 24);
    out[1] = (uint8_t)(v >> 16);
    out[2] = (uint8_t)(v >> 8);
    out[3] = (uint8_t)(v);
}

uint32_t bytes_to_u32(const uint8_t in[4]) {
    return ((uint32_t)in[0] << 24) | 
           ((uint32_t)in[1] << 16) | 
           ((uint32_t)in[2] << 8) | 
           ((uint32_t)in[3]);
}
