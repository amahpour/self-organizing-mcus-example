#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include "../shared/core/hal.h"
#include "../shared/core/bus_interface.h"
#include "../shared/core/node.h"

typedef struct {
    Node node;
    Bus* bus;
    uint8_t index;
    int running;
    pthread_t thread;
} ThreadedNode;

static void* node_thread(void* arg) {
    ThreadedNode* tn = (ThreadedNode*)arg;
    
    node_begin(&tn->node);
    
    while (tn->running) {
        node_service(&tn->node);
        usleep(10000); // 10ms service interval
    }
    
    return NULL;
}

int main(int argc, char** argv) {
    int num_nodes = 3;
    if (argc >= 2) {
        num_nodes = atoi(argv[1]);
        if (num_nodes < 1) num_nodes = 1;
        if (num_nodes > 16) num_nodes = 16;
    }
    
    printf("Starting simulation with %d nodes...\n", num_nodes);
    
    hal_init();
    
    if (bus_global_init((uint8_t)num_nodes) != 0) {
        fprintf(stderr, "Failed to initialize bus system\n");
        return 1;
    }
    
    ThreadedNode* nodes = (ThreadedNode*)calloc((size_t)num_nodes, sizeof(ThreadedNode));
    if (!nodes) {
        fprintf(stderr, "Memory allocation failed\n");
        return 1;
    }
    
    // Create nodes and buses
    for (int i = 0; i < num_nodes; ++i) {
        if (bus_create(&nodes[i].bus, (uint8_t)i, 0, 0) != 0) {
            fprintf(stderr, "Failed to create bus for node %d\n", i);
            return 1;
        }
        
        node_init(&nodes[i].node, nodes[i].bus, (uint8_t)i);
        nodes[i].index = (uint8_t)i;
        nodes[i].running = 1;
        
        if (pthread_create(&nodes[i].thread, NULL, node_thread, &nodes[i]) != 0) {
            fprintf(stderr, "Failed to create thread for node %d\n", i);
            return 1;
        }
    }
    
    // Let simulation run
    sleep(3);
    
    // Shutdown
    for (int i = 0; i < num_nodes; ++i) {
        nodes[i].running = 0;
        pthread_join(nodes[i].thread, NULL);
        bus_destroy(nodes[i].bus);
    }
    
    bus_global_shutdown();
    free(nodes);
    
    return 0;
}
