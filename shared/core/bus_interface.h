/**
 * @file bus_interface.h
 * @brief Communication bus abstraction for cross-platform messaging
 *
 * This header defines an abstract interface for inter-node communication.
 * Different platforms implement this interface using their available
 * communication mechanisms (UART, in-process queues, WiFi, etc.).
 *
 * Design Principles:
 * - Opaque bus handle prevents platform-specific coupling
 * - Simple send/receive API with timeout support
 * - Frame-based messaging with built-in validation
 * - Support for both point-to-point and broadcast communication
 */

#ifndef BUS_INTERFACE_H
#define BUS_INTERFACE_H

#include <stdint.h>

#include "proto.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Opaque bus handle
 *
 * This structure is defined by each platform implementation and contains
 * the platform-specific state needed for communication. Core code never
 * accesses the contents directly - only through the interface functions.
 */
typedef struct Bus Bus;

/**
 * @brief Initialize global bus subsystem (platform-specific)
 *
 * Some platforms (like simulation) need global initialization before
 * individual buses can be created. Others (like Arduino UART) don't
 * need this. Safe to call on all platforms.
 *
 * @param max_nodes Maximum number of nodes that will use the bus
 * @return 0 on success, negative on error
 *
 * Platform Examples:
 * - Simulation: Allocate shared message queues for all nodes
 * - Arduino: No-op (UART doesn't need global state)
 * - WiFi: Initialize networking stack
 */
int bus_global_init(uint8_t max_nodes);

/**
 * @brief Shutdown global bus subsystem
 *
 * Clean up any global resources allocated by bus_global_init().
 * Call this during system shutdown after all individual buses
 * have been destroyed.
 *
 * Platform Examples:
 * - Simulation: Free shared message queues, cleanup threads
 * - Arduino: No-op
 * - WiFi: Shutdown networking stack
 */
void bus_global_shutdown(void);

/**
 * @brief Create and initialize a communication bus instance
 *
 * Creates a new bus instance for a specific node. The meaning of
 * rx_pin and tx_pin depends on the platform - they may be hardware
 * pins, logical addresses, or ignored entirely.
 *
 * @param bus Output pointer to receive the created bus handle
 * @param node_index Unique index for this node (0, 1, 2, ...)
 * @param rx_pin Platform-specific receive pin/address
 * @param tx_pin Platform-specific transmit pin/address
 * @return 0 on success, negative on error
 *
 * Platform Examples:
 * - Simulation: Create message queue for this node index
 * - Arduino: Initialize SoftwareSerial with specified pins
 * - ESP32: Setup UART with specified GPIO pins
 */
int bus_create(Bus** bus, uint8_t node_index, uint8_t rx_pin, uint8_t tx_pin);

/**
 * @brief Destroy a bus instance and free its resources
 *
 * Cleans up platform-specific resources associated with this bus.
 * The bus handle becomes invalid after this call.
 *
 * @param bus Bus handle to destroy (may be NULL)
 *
 * Platform Examples:
 * - Simulation: No cleanup needed (global queues persist)
 * - Arduino: Delete SoftwareSerial object
 * - ESP32: Disable UART peripheral
 */
void bus_destroy(Bus* bus);

/**
 * @brief Send a frame over the bus
 *
 * Transmits a protocol frame to other nodes on the bus. The frame
 * should be properly finalized (proto_finalize()) before sending.
 * This function may implement broadcast or point-to-point delivery
 * depending on the platform.
 *
 * @param bus Bus handle to send on
 * @param frame Pointer to frame to transmit
 * @return 1 on success, 0 on failure
 *
 * Platform Examples:
 * - Simulation: Broadcast frame to all node message queues
 * - Arduino: Serialize frame to UART TX
 * - ESP32: Send frame over WiFi broadcast
 */
int bus_send(Bus* bus, const Frame* frame);

/**
 * @brief Receive a frame from the bus with timeout
 *
 * Attempts to receive a protocol frame from other nodes. This function
 * blocks for up to timeout_ms milliseconds waiting for a frame.
 * Received frames are automatically validated before being returned.
 *
 * @param bus Bus handle to receive on
 * @param frame Pointer to frame structure to populate
 * @param timeout_ms Maximum time to wait in milliseconds (0 = non-blocking)
 * @return 1 if frame received, 0 if timeout/error
 *
 * Platform Examples:
 * - Simulation: Pop frame from node's message queue with timeout
 * - Arduino: Parse incoming UART bytes into frame with timeout
 * - ESP32: Receive WiFi packet and deserialize to frame
 */
int bus_recv(Bus* bus, Frame* frame, uint16_t timeout_ms);

#ifdef __cplusplus
}
#endif

#endif  // BUS_INTERFACE_H
