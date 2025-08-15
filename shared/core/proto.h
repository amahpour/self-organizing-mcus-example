/**
 * @file proto.h
 * @brief Communication protocol definitions for self-organizing microcontrollers
 *
 * This header defines the wire protocol used for communication between nodes
 * in the distributed system. The protocol is designed to be simple, robust,
 * and suitable for embedded systems with limited bandwidth and processing power.
 *
 * Protocol Features:
 * - Fixed-size frame header with variable payload
 * - Simple XOR checksum for error detection
 * - Big-endian byte ordering for cross-platform compatibility
 * - Compact 5-13 byte frames (header + 0-8 byte payload)
 */

#ifndef PROTO_H
#define PROTO_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Start-of-frame marker to identify frame boundaries */
#define SOF 0xAA

/** Maximum payload size in bytes (keeps frames small for embedded systems) */
#define MAX_PAYLOAD_SIZE 8

/**
 * @brief Message types used in the distributed coordination protocol
 *
 * The protocol uses a small set of message types to minimize complexity
 * while providing all necessary functionality for coordinator election
 * and member management.
 */
typedef enum {
    MSG_HELLO = 1,    /**< Member announces presence to the network */
    MSG_CLAIM = 2,    /**< Node claims coordinator role (includes tie-break nonce) */
    MSG_JOIN = 3,     /**< Member requests ID assignment (includes unique nonce) */
    MSG_ASSIGN = 4,   /**< Coordinator assigns ID to member (echoes JOIN nonce) */
    MSG_HEARTBEAT = 5 /**< Coordinator periodic heartbeat (future extension) */
} MessageType;

/**
 * @brief Wire protocol frame structure
 *
 * Frame Format (5-13 bytes total):
 * [SOF][Type][Source][PayloadLen][Payload...][Checksum]
 *  1B   1B    1B      1B         0-8B        1B
 *
 * All multi-byte values use big-endian (network) byte order.
 */
typedef struct {
    uint8_t sof;                       /**< Start-of-frame marker (always SOF) */
    uint8_t type;                      /**< Message type (MessageType enum) */
    uint8_t source;                    /**< Source node ID (0 = unassigned) */
    uint8_t payload_len;               /**< Payload length (0-MAX_PAYLOAD_SIZE) */
    uint8_t payload[MAX_PAYLOAD_SIZE]; /**< Variable payload data */
    uint8_t checksum;                  /**< XOR checksum of type+source+len+payload */
} Frame;

/**
 * @brief Compute XOR checksum for frame validation
 *
 * The checksum covers all fields except SOF and checksum itself.
 * This provides basic error detection for transmission errors.
 *
 * @param f Pointer to frame to compute checksum for
 * @return 8-bit XOR checksum value
 */
uint8_t proto_compute_checksum(const Frame* f);

/**
 * @brief Finalize frame before transmission
 *
 * Sets the SOF marker, clamps payload length, and computes checksum.
 * Call this function on every frame before sending.
 *
 * @param f Pointer to frame to finalize
 */
void proto_finalize(Frame* f);

/**
 * @brief Validate received frame for correctness
 *
 * Checks SOF marker, payload length bounds, and checksum validity.
 * Use this to filter out corrupted or malformed frames.
 *
 * @param f Pointer to frame to validate
 * @return 1 if frame is valid, 0 if corrupted/malformed
 */
int proto_is_valid(const Frame* f);

/**
 * @brief Convert 32-bit value to big-endian byte array
 *
 * Serializes a 32-bit unsigned integer into network byte order
 * for transmission in frame payloads.
 *
 * @param v 32-bit value to convert
 * @param out Pointer to 4-byte output array
 */
void u32_to_bytes(uint32_t v, uint8_t out[4]);

/**
 * @brief Convert big-endian byte array to 32-bit value
 *
 * Deserializes a byte array from network byte order back
 * into a 32-bit unsigned integer.
 *
 * @param in Pointer to 4-byte input array
 * @return Reconstructed 32-bit value
 */
uint32_t bytes_to_u32(const uint8_t in[4]);

#ifdef __cplusplus
}
#endif

#endif  // PROTO_H
