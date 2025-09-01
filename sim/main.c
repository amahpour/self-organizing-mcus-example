/* Enable POSIX.1-2008 features for usleep() and other functions */
#define _POSIX_C_SOURCE 200809L
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "../shared/core/bus_interface.h"
#include "../shared/core/hal.h"
#include "../shared/core/node.h"

/**
 * @brief Structure representing a node running in its own thread
 * 
 * This structure wraps a Node with threading capabilities for simulation.
 * Each ThreadedNode runs independently in its own pthread.
 */
typedef struct {
    Node node;          /* The actual node instance */
    Bus* bus;           /* Bus interface for communication */
    uint8_t index;      /* Unique identifier for this node */
    int running;        /* Flag to control thread execution (1=running, 0=stop) */
    pthread_t thread;   /* POSIX thread handle */
} ThreadedNode;

/**
 * @brief Thread function that runs a single node's main loop
 * @param arg Pointer to ThreadedNode structure (cast from void*)
 * @return NULL (required by pthread interface)
 * 
 * This function runs in its own thread and continuously services a node.
 * It initializes the node, then runs the service loop until told to stop.
 */
static void* node_thread(void* arg) {
    ThreadedNode* tn = (ThreadedNode*) arg;  /* Cast void* back to ThreadedNode* */

    /* Initialize the node (similar to Arduino setup() function) */
    node_begin(&tn->node);

    /* Main service loop (similar to Arduino loop() function) */
    while (tn->running) {
        node_service(&tn->node);  /* Process node logic and communications */
        usleep(10000);            /* Sleep for 10ms to simulate real-time behavior */
    }

    return NULL;  /* Thread cleanup - return NULL to indicate success */
}

/**
 * @brief Main simulation entry point
 * @param argc Number of command line arguments
 * @param argv Array of command line argument strings
 * @return 0 on success, 1 on failure
 * 
 * Creates and runs a multi-threaded simulation of interconnected nodes.
 * Each node runs in its own thread and can communicate with others via a shared bus.
 * Usage: ./sim [num_nodes] (default: 3 nodes, max: 16)
 */
int main(int argc, char** argv) {
    /* Default to 3 nodes if no argument provided */
    int num_nodes = 3;
    
    /* Parse command line argument for number of nodes */
    if (argc >= 2) {
        num_nodes = atoi(argv[1]);  /* Convert string to integer */
        /* Clamp to valid range [1, 16] */
        if (num_nodes < 1)
            num_nodes = 1;
        if (num_nodes > 16)
            num_nodes = 16;
    }

    printf("Starting simulation with %d nodes...\n", num_nodes);

    /* Initialize hardware abstraction layer (HAL) */
    hal_init();

    /* Initialize the global bus system that connects all nodes */
    if (bus_global_init((uint8_t) num_nodes) != 0) {
        fprintf(stderr, "Failed to initialize bus system\n");
        return 1;
    }

    /* Allocate memory for all node structures (initialized to zero) */
    ThreadedNode* nodes = (ThreadedNode*) calloc((size_t) num_nodes, sizeof(ThreadedNode));
    if (!nodes) {
        fprintf(stderr, "Memory allocation failed\n");
        return 1;
    }

    /* Create and initialize each node with its own bus and thread */
    for (int i = 0; i < num_nodes; ++i) {
        /* Create a bus interface for this node (parameters: bus_ptr, node_id, tx_pin, rx_pin) */
        if (bus_create(&nodes[i].bus, (uint8_t) i, 0, 0) != 0) {
            fprintf(stderr, "Failed to create bus for node %d\n", i);
            return 1;
        }

        /* Initialize the node with its bus and unique ID */
        node_init(&nodes[i].node, nodes[i].bus, (uint8_t) i);
        nodes[i].index = (uint8_t) i;  /* Store the node index for reference */
        nodes[i].running = 1;          /* Set running flag to start the node */

        /* Create a new thread to run this node independently */
        if (pthread_create(&nodes[i].thread, NULL, node_thread, &nodes[i]) != 0) {
            fprintf(stderr, "Failed to create thread for node %d\n", i);
            return 1;
        }
    }

    /* Let the simulation run for 3 seconds */
    printf("Simulation running...\n");
    sleep(3);

    /* Graceful shutdown sequence */
    printf("Shutting down simulation...\n");
    for (int i = 0; i < num_nodes; ++i) {
        /* Signal the node thread to stop */
        nodes[i].running = 0;
        
        /* Wait for the thread to finish (blocking call) */
        pthread_join(nodes[i].thread, NULL);
        
        /* Clean up the bus resources for this node */
        bus_destroy(nodes[i].bus);
    }

    /* Clean up global resources */
    bus_global_shutdown();  /* Shutdown the global bus system */
    free(nodes);           /* Free the allocated node array */

    printf("Simulation completed successfully.\n");
    return 0;  /* Success */
}

