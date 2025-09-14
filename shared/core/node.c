/**
 * @file node.c
 * @brief Core node state machine implementation for self-organizing microcontrollers
 *
 * This file implements the heart of the distributed system - the node state machine
 * that handles coordinator election, member joining, and ID assignment. The code is
 * platform-agnostic and contains zero ifdefs.
 *
 * State Machine:
 * - SEEKING: Node is looking for a coordinator or trying to become one
 * - COORDINATOR: Node assigns IDs to new members and manages the network
 * - MEMBER: Node has received an ID and participates in the network
 */

#include "node.h"

#include <stdio.h>
#include <string.h>

#include "hal.h"

/**
 * @brief Check if coordinator has already seen a JOIN request nonce (deduplication)
 *
 * This prevents the coordinator from assigning multiple IDs to the same node
 * that might retransmit JOIN messages. Uses a simple ring buffer for storage.
 *
 * @param n Pointer to the coordinator node
 * @param nonce The JOIN request nonce to check
 * @return 1 if nonce was already seen, 0 if it's new (and now recorded)
 */
static int coordinator_seen_nonce(Node* n, uint32_t nonce) {
    // Check if we've seen this nonce before
    for (uint8_t i = 0; i < n->seen_count; ++i) {
        if (n->seen_join_nonce[i] == nonce) {
            return 1;  // Already seen - duplicate request
        }
    }

    // New nonce - record it for future deduplication
    if (n->seen_count < NODE_MAX_DEDUP) {
        // Still have space in the buffer
        n->seen_join_nonce[n->seen_count++] = nonce;
    } else {
        // Buffer full - overwrite oldest entry (ring buffer behavior)
        n->seen_join_nonce[n->seen_count % NODE_MAX_DEDUP] = nonce;
        n->seen_count = (uint8_t) ((n->seen_count + 1) % NODE_MAX_DEDUP);
    }
    return 0;  // New nonce, now recorded
}

/**
 * @brief Create and finalize a protocol frame for transmission
 *
 * This helper function constructs a properly formatted frame with the given
 * parameters and automatically computes the checksum.
 *
 * @param f Pointer to frame structure to populate
 * @param type Message type (HELLO, CLAIM, JOIN, ASSIGN, etc.)
 * @param source Source node ID (0 if unknown/unassigned)
 * @param payload Pointer to payload data (can be NULL)
 * @param len Length of payload data in bytes
 */
static void make_frame(Frame* f, MessageType type, uint8_t source, const void* payload,
                       uint8_t len) {
    // Clear the frame to ensure no garbage data
    memset(f, 0, sizeof(*f));

    // Set frame fields
    f->type = (uint8_t) type;
    f->source = source;
    f->payload_len = len > MAX_PAYLOAD_SIZE ? MAX_PAYLOAD_SIZE : len;

    // Copy payload if provided
    if (payload && f->payload_len) {
        memcpy(f->payload, payload, f->payload_len);
    }

    // Finalize frame (sets SOF and computes checksum)
    proto_finalize(f);
}

/**
 * @brief Initialize a node with its bus connection and instance index
 *
 * This function sets up the basic node structure but doesn't start the
 * state machine. Call node_begin() to start the coordinator election process.
 *
 * @param n Pointer to node structure to initialize
 * @param bus Pointer to the communication bus interface
 * @param instance_index Unique index for this node instance (used for startup jitter)
 */
void node_init(Node* n, Bus* bus, uint8_t instance_index) {
    // Clear all node state to ensure clean initialization
    memset(n, 0, sizeof(*n));

    // Set up basic node parameters
    n->bus = bus;
    n->instance_index = instance_index;
}

/**
 * @brief Start the node and begin the coordinator election process
 *
 * This function implements the core startup logic:
 * 1. Add startup jitter to avoid simultaneous startup conflicts
 * 2. Listen for existing coordinator CLAIM messages
 * 3. If no coordinator exists, attempt to claim coordinator role
 * 4. Handle tie-breaking if multiple nodes claim simultaneously
 * 5. If not coordinator, begin the member joining process
 *
 * @param n Pointer to the initialized node
 */
void node_begin(Node* n) {
    // Add startup jitter to prevent all nodes from starting simultaneously
    // Each node waits 150ms * instance_index before proceeding
    hal_delay((uint32_t) n->instance_index * 150);

    // Initialize node state for the election process
    n->role = NODE_SEEKING;
    n->assigned_id = 0;
    n->random_nonce = hal_random32();  // For tie-breaking in coordinator election
    n->seen_count = 0;
    n->last_join_ms = 0;
    n->in_election = 1;  // Prevent node_service() from consuming messages during election

    // Phase 1: Listen for existing CLAIM messages (500ms window - increased for reliability)
    // This detects if another node is already trying to become coordinator
    Frame in;
    int heard_claim = 0;
    uint32_t listen_start = hal_millis();
    uint32_t listen_end = listen_start + 1000;
    uint32_t last_debug = 0;

    while (hal_millis() < listen_end) {
        uint32_t now = hal_millis();
        
        // Debug output every 100ms during listen phase
        if (now - last_debug >= 100) {
            char debug_msg[64];
            snprintf(debug_msg, sizeof(debug_msg), "DEBUG: Listening for CLAIM... elapsed=%ums", now - listen_start);
            hal_log(debug_msg);
            last_debug = now;
        }
        
        if (bus_recv(n->bus, &in, 50)) {
            if (proto_is_valid(&in) && in.type == MSG_CLAIM) {
                hal_log("DEBUG: *** HEARD CLAIM MESSAGE! *** Breaking out of listen phase");
                heard_claim = 1;
                break;
            } else {
                char debug_msg[64];
                snprintf(debug_msg, sizeof(debug_msg), "DEBUG: Received non-CLAIM frame during listen: type=%d", in.type);
                hal_log(debug_msg);
            }
        }
        hal_yield();  // Allow other tasks to run while waiting
    }
    
    char debug_msg[80];
    snprintf(debug_msg, sizeof(debug_msg), "DEBUG: Listen phase complete. Duration=%ums, heard_claim=%d", 
             hal_millis() - listen_start, heard_claim);
    hal_log(debug_msg);

    // Phase 2: Coordinator Election
    if (!heard_claim) {
        hal_log("DEBUG: No CLAIM heard - proceeding to send our CLAIM");
        // No existing coordinator detected - attempt to claim the role
        uint8_t payload[4];
        u32_to_bytes(n->random_nonce, payload);
        Frame claim;
        make_frame(&claim, MSG_CLAIM, 0, payload, 4);
        hal_log("DEBUG: About to send CLAIM message");
        bus_send(n->bus, &claim);

        char msg[64];
        snprintf(msg, sizeof(msg), "Node[%u] CLAIM nonce=%u", n->instance_index, n->random_nonce);
        hal_log(msg);

        // Phase 3: Conflict Detection Window (1000ms)
        // If another node claims with a higher nonce, we yield to them
        // Extended timeout for ATmega328P: coordinator takes ~60-90ms to respond + bus delays
        int lost = 0;
        uint32_t conflict_end = hal_millis() + 1000;

        while (hal_millis() < conflict_end) {
            if (bus_recv(n->bus, &in, 50) && proto_is_valid(&in) && in.type == MSG_CLAIM &&
                in.payload_len >= 4) {
                uint32_t other_nonce = bytes_to_u32(in.payload);
                // If we hear a CLAIM from source ID 1 (the canonical coordinator ID), yield immediately.
                // This prevents taking over when a coordinator is already established, regardless of nonce.
                if (in.source == 1) {
                    lost = 1;
                    break;
                }
                if (other_nonce > n->random_nonce) {
                    // Another node has a higher nonce - they win
                    lost = 1;
                    break;
                }
            }
            hal_yield();
        }

        // Phase 4: Coordinator Role Assignment
        if (!lost) {
            // We won the election - become coordinator
            n->role = NODE_COORDINATOR;
            n->assigned_id = 1;     // Coordinator always gets ID 1
            n->next_assign_id = 2;  // Next ID to assign to members

            snprintf(msg, sizeof(msg), "Node[%u] → COORDINATOR (ID=1)", n->instance_index);
            hal_log(msg);
        }
    }

    // Phase 5: Member Joining Process
    if (n->role == NODE_SEEKING) {
        // We didn't become coordinator - join as a member

        // Send HELLO to announce our presence
        Frame hello;
        make_frame(&hello, MSG_HELLO, 0, NULL, 0);
        bus_send(n->bus, &hello);
        hal_log("HELLO");

        // Send JOIN request with a unique nonce
        n->join_nonce = hal_random32();
        uint8_t payload[4];
        u32_to_bytes(n->join_nonce, payload);
        Frame join;
        make_frame(&join, MSG_JOIN, 0, payload, 4);
        bus_send(n->bus, &join);
        n->last_join_ms = hal_millis();

        char msg[64];
        snprintf(msg, sizeof(msg), "JOIN (nonce=%u)", n->join_nonce);
        hal_log(msg);
    }
    
    n->in_election = 0;  // Allow node_service() to process messages now
}

/**
 * @brief Service the node state machine (call this regularly in main loop)
 *
 * This function handles ongoing node operations:
 * - For coordinators: Process JOIN requests and assign IDs
 * - For members: Handle ASSIGN responses and retry JOIN if needed
 * - For seeking nodes: Continue trying to join until successful
 *
 * Call this function regularly (e.g., every 10-50ms) to keep the node responsive.
 *
 * @param n Pointer to the node to service
 */
void node_service(Node* n) {
    Frame in;

    // Don't process messages during coordinator election to prevent race conditions
    if (n->in_election) {
        return;
    }

    // Process any incoming messages with a short timeout to stay responsive
    if (bus_recv(n->bus, &in, 50) && proto_is_valid(&in)) {
        char debug_msg[64];
        snprintf(debug_msg, sizeof(debug_msg), "DEBUG: node_service received frame type=%d from source=%d", in.type, in.source);
        hal_log(debug_msg);
        if (n->role == NODE_COORDINATOR) {
            // Coordinator Logic: Handle CLAIM messages from new nodes trying to become coordinator
            if (in.type == MSG_CLAIM && in.payload_len >= 4) {
                uint32_t incoming_nonce = bytes_to_u32(in.payload);
                char nonce_msg[80];
                snprintf(nonce_msg, sizeof(nonce_msg), "DEBUG: COORDINATOR comparing nonces - incoming=%u, ours=%u", incoming_nonce, n->random_nonce);
                hal_log(nonce_msg);
                
                // COORDINATOR ALWAYS defends its position - never steps down after election
                hal_log("DEBUG: CLAIM received - defending coordinator position");
                uint8_t payload[4];
                u32_to_bytes(n->random_nonce, payload);
                Frame claim;
                make_frame(&claim, MSG_CLAIM, 1, payload, 4);
                bus_send(n->bus, &claim);
            }
            // Handle JOIN requests from new members
            else if (in.type == MSG_JOIN && in.payload_len >= 4) {
                uint32_t nonce = bytes_to_u32(in.payload);

                // Check if we've already assigned an ID for this nonce (deduplication)
                if (coordinator_seen_nonce(n, nonce)) {
                    return;  // Already handled this request
                }

                // Assign the next available ID to this member
                uint8_t id = n->next_assign_id++;
                uint8_t payload[5];
                payload[0] = id;                     // Assigned ID
                memcpy(&payload[1], in.payload, 4);  // Echo back the JOIN nonce

                Frame assign;
                make_frame(&assign, MSG_ASSIGN, 1, payload, 5);
                bus_send(n->bus, &assign);

                char msg[32];
                snprintf(msg, sizeof(msg), "ASSIGN → id=%u", id);
                hal_log(msg);
            }

        } else if (n->role == NODE_SEEKING) {
            // Member Logic: Handle ASSIGN responses from coordinator
            if (in.type == MSG_ASSIGN && in.payload_len >= 5) {
                uint8_t assigned = in.payload[0];
                uint32_t echoed = bytes_to_u32(&in.payload[1]);

                // Verify this ASSIGN is for us by checking the echoed nonce
                if (echoed == n->join_nonce) {
                    // Successfully assigned an ID - become a member
                    n->assigned_id = assigned;
                    n->role = NODE_MEMBER;

                    char msg[64];
                    snprintf(msg, sizeof(msg), "ASSIGN received → MEMBER (ID=%u)", n->assigned_id);
                    hal_log(msg);
                }
            }
        }
        // NODE_MEMBER nodes don't need to process messages in this basic implementation
    }

    // Retry Logic: If still seeking and haven't heard back, retry JOIN periodically
    if (n->role == NODE_SEEKING && (hal_millis() - n->last_join_ms) >= 250) {
        // Resend JOIN request every 250ms until we get an ASSIGN response
        uint8_t payload[4];
        u32_to_bytes(n->join_nonce, payload);
        Frame join;
        make_frame(&join, MSG_JOIN, 0, payload, 4);
        bus_send(n->bus, &join);
        n->last_join_ms = hal_millis();
    }
}
