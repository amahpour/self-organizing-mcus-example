#include <stdio.h>
#include <stdlib.h>
#include "../shared/hal.h"
#include "../shared/bus.h"
#include "../shared/node.h"

int main(int argc, char** argv) {
    int n = 3;
    if (argc >= 2) {
        n = atoi(argv[1]);
        if (n < 1) n = 1;
        if (n > 32) n = 32;
    }

    hal_init();
    bus_global_init((uint8_t)n);

    Bus* buses = (Bus*)calloc((size_t)n, sizeof(Bus));
    Node* nodes = (Node*)calloc((size_t)n, sizeof(Node));
    if (!buses || !nodes) return 1;

    for (int i = 0; i < n; ++i) {
        bus_init(&buses[i], (uint8_t)i, 0, 0);
        node_init(&nodes[i], &buses[i], (uint8_t)i);
        node_begin(&nodes[i]);
    }

    uint32_t end = hal_millis() + 3000;
    while (hal_millis() < end) {
        for (int i = 0; i < n; ++i) {
            node_service(&nodes[i]);
        }
        hal_delay(10);
    }

    bus_global_shutdown();
    free(buses);
    free(nodes);
    return 0;
}
