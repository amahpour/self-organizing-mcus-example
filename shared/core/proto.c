/**
 * @file proto.c
 * @brief Protocol implementation for self-organizing microcontroller communication
 *
 * This file implements the core protocol functions for frame validation,
 * checksum computation, and data serialization. The protocol is designed
 * to be simple, robust, and suitable for embedded systems with limited
 * processing power.
 *
 * Frame Format:
 * [SOF][Type][Source][PayloadLen][Payload...][Checksum]
 *  1B   1B    1B      1B         0-8B        1B
 */

#include "proto.h"

/**
 * @brief Compute XOR checksum for a protocol frame
 *
 * The checksum covers all fields except SOF and the checksum field itself.
 * This provides basic error detection for transmission errors. XOR is chosen
 * for simplicity and speed on microcontrollers.
 *
 * @param f Pointer to the frame to compute checksum for
 * @return 8-bit XOR checksum value
 */
uint8_t proto_compute_checksum(const Frame* f) {
    uint8_t checksum = 0;

    // XOR all header fields
    checksum ^= f->type;
    checksum ^= f->source;
    checksum ^= f->payload_len;

    // XOR all payload bytes
    for (uint8_t i = 0; i < f->payload_len; ++i) {
        checksum ^= f->payload[i];
    }

    return checksum;
}

/**
 * @brief Finalize a frame by setting SOF, clamping payload length, and computing checksum
 *
 * This function should be called on every frame before transmission. It ensures
 * the frame is properly formatted and has a valid checksum.
 *
 * @param f Pointer to the frame to finalize
 */
void proto_finalize(Frame* f) {
    // Set the start-of-frame marker
    f->sof = SOF;

    // Clamp payload length to maximum allowed size
    if (f->payload_len > MAX_PAYLOAD_SIZE) {
        f->payload_len = MAX_PAYLOAD_SIZE;
    }

    // Compute and set the checksum
    f->checksum = proto_compute_checksum(f);
}

/**
 * @brief Validate a received frame for correctness
 *
 * This function checks that a received frame has the correct format and
 * a valid checksum. Use this to filter out corrupted or malformed frames.
 *
 * @param f Pointer to the frame to validate
 * @return 1 if frame is valid, 0 if corrupted or malformed
 */
int proto_is_valid(const Frame* f) {
    // Check start-of-frame marker
    if (f->sof != SOF) {
        return 0;  // Invalid SOF
    }

    // Check payload length is within bounds
    if (f->payload_len > MAX_PAYLOAD_SIZE) {
        return 0;  // Payload too large
    }

    // Verify checksum matches computed value
    return proto_compute_checksum(f) == f->checksum;
}

/**
 * @brief Convert a 32-bit unsigned integer to big-endian byte array
 *
 * This function serializes a 32-bit value into a byte array for transmission.
 * Big-endian format is used for network byte order consistency.
 *
 * Example: 0x12345678 → [0x12, 0x34, 0x56, 0x78]
 *
 * @param v The 32-bit value to convert
 * @param out Pointer to 4-byte output array
 */
void u32_to_bytes(uint32_t v, uint8_t out[4]) {
    out[0] = (uint8_t) (v >> 24);  // Most significant byte
    out[1] = (uint8_t) (v >> 16);
    out[2] = (uint8_t) (v >> 8);
    out[3] = (uint8_t) (v);  // Least significant byte
}

/**
 * @brief Convert a big-endian byte array to 32-bit unsigned integer
 *
 * This function deserializes a byte array back into a 32-bit value after
 * reception. This is the inverse of u32_to_bytes().
 *
 * Example: [0x12, 0x34, 0x56, 0x78] → 0x12345678
 *
 * @param in Pointer to 4-byte input array
 * @return The reconstructed 32-bit value
 */
uint32_t bytes_to_u32(const uint8_t in[4]) {
    return ((uint32_t) in[0] << 24) |  // Most significant byte
           ((uint32_t) in[1] << 16) | ((uint32_t) in[2] << 8) |
           ((uint32_t) in[3]);  // Least significant byte
}
