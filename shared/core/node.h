/**
 * @file node.h
 * @brief Node state machine interface for self-organizing microcontrollers
 *
 * This header defines the core node data structures and API for implementing
 * a distributed coordinator election and member management system. The design
 * is completely platform-agnostic with zero preprocessor conditionals.
 *
 * Key Concepts:
 * - Nodes start in SEEKING state and either become COORDINATOR or MEMBER
 * - Coordinator election uses random nonces for tie-breaking
 * - Members retry JOIN requests until they receive an ID assignment
 * - All communication happens through the abstract bus interface
 */

#ifndef NODE_H
#define NODE_H

#include <stdint.h>

#include "bus_interface.h"
#include "proto.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Node roles in the distributed system
 *
 * The state machine progresses: SEEKING → (COORDINATOR | MEMBER)
 * Once a role is assigned, it typically doesn't change during the session.
 */
typedef enum {
    NODE_SEEKING = 0,     /**< Node is looking for coordinator or trying to become one */
    NODE_COORDINATOR = 1, /**< Node manages the network and assigns IDs to members */
    NODE_MEMBER = 2       /**< Node has received an ID and participates in the network */
} NodeRole;

/** Maximum number of JOIN request nonces to remember for deduplication */
#define NODE_MAX_DEDUP 32

/**
 * @brief Complete node state structure
 *
 * This structure contains all the state needed for a node to participate
 * in the distributed system. It's designed to be compact and suitable
 * for resource-constrained microcontrollers.
 */
typedef struct {
    // Core node identity and communication
    Bus* bus;               /**< Communication bus interface */
    uint8_t instance_index; /**< Unique instance identifier for startup jitter */
    NodeRole role;          /**< Current role in the distributed system */
    uint8_t assigned_id;    /**< Network ID (0 = unassigned, 1+ = assigned) */

    // Coordinator election state
    uint32_t random_nonce; /**< Random nonce for coordinator election tie-breaking */
    uint8_t in_election;   /**< Flag to prevent node_service() from consuming messages during election */

    // Coordinator-specific state
    uint8_t next_assign_id; /**< Next ID to assign to joining members (starts at 2) */

    // JOIN request deduplication (prevents double-assignment)
    uint32_t seen_join_nonce[NODE_MAX_DEDUP]; /**< Ring buffer of seen JOIN nonces */
    uint8_t seen_count;                       /**< Number of nonces in buffer */

    // Member-specific state
    uint32_t join_nonce;   /**< Unique nonce for our JOIN request */
    uint32_t last_join_ms; /**< Timestamp of last JOIN transmission (for retry logic) */
} Node;

/**
 * @brief Initialize a node with its communication bus and instance identifier
 *
 * This function prepares the node structure but doesn't start the state machine.
 * The instance_index is used to add startup jitter to prevent simultaneous
 * coordinator claims when multiple nodes boot at the same time.
 *
 * @param n Pointer to the node structure to initialize
 * @param bus Pointer to the communication bus interface
 * @param instance_index Unique index for this node (0, 1, 2, ...)
 */
void node_init(Node* n, Bus* bus, uint8_t instance_index);

/**
 * @brief Start the node and begin the coordinator election process
 *
 * This function implements the core distributed algorithm:
 * 1. Listen for existing coordinator announcements
 * 2. If none found, attempt to claim coordinator role
 * 3. Handle tie-breaking with other claimants using random nonces
 * 4. If not coordinator, begin member joining process
 *
 * This function blocks for several hundred milliseconds during the election
 * process, so call it during system initialization, not in tight loops.
 *
 * @param n Pointer to the initialized node
 */
void node_begin(Node* n);

/**
 * @brief Service the node state machine (call regularly in main loop)
 *
 * This function handles ongoing node operations based on current role:
 * - COORDINATOR: Process JOIN requests and assign unique IDs
 * - MEMBER: Handle ASSIGN responses from coordinator
 * - SEEKING: Retry JOIN requests until assignment received
 *
 * This function is non-blocking and should be called regularly (every 10-50ms)
 * to maintain responsive communication with other nodes.
 *
 * @param n Pointer to the node to service
 */
void node_service(Node* n);

#ifdef __cplusplus
}
#endif

#endif  // NODE_H
