#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "bus.h"
#include "node.h"

int main(int argc, char** argv) {
    int n = 3;
    if (argc >= 2) {
        n = atoi(argv[1]);
        if (n < 1) n = 1;
        if (n > 32) n = 32;
    }

    srand((unsigned)time(NULL));

    if (bus_init((size_t)n) != 0) {
        fprintf(stderr, "Failed to init bus\n");
        return 1;
    }

    Node* nodes = (Node*)calloc((size_t)n, sizeof(Node));
    if (!nodes) {
        fprintf(stderr, "Alloc failure\n");
        return 1;
    }

    for (int i = 0; i < n; ++i) {
        if (node_start(&nodes[i], (uint8_t)i) != 0) {
            fprintf(stderr, "Failed to start node %d\n", i);
            return 1;
        }
    }

    // Let the simulation run for a few seconds
    sleep(3);

    for (int i = 0; i < n; ++i) {
        node_stop(&nodes[i]);
    }

    bus_shutdown();
    free(nodes);

    return 0;
}
