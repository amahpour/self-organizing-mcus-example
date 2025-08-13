#include "bus.h"
#include "hal.h"
#include <string.h>

#define BUS_MAX_NODES 32
#define BUS_RING_CAPACITY 64

typedef struct {
    Frame buffer[BUS_RING_CAPACITY];
    uint8_t head, tail, count;
} Queue;

static uint8_t g_max_nodes;
static Queue g_queues[BUS_MAX_NODES];

int bus_global_init(uint8_t max_nodes) {
    if (max_nodes > BUS_MAX_NODES) max_nodes = BUS_MAX_NODES;
    g_max_nodes = max_nodes;
    memset(g_queues, 0, sizeof(g_queues));
    return 0;
}

void bus_global_shutdown(void) {
    // nothing
}

int bus_init(Bus* bus, uint8_t node_index, uint8_t rx_pin, uint8_t tx_pin) {
    (void)rx_pin; (void)tx_pin;
    bus->index = node_index;
    return 0;
}

static void enqueue(Queue* q, const Frame* f) {
    if (q->count == BUS_RING_CAPACITY) {
        q->head = (q->head + 1) % BUS_RING_CAPACITY;
        q->count--;
    }
    q->buffer[q->tail] = *f;
    q->tail = (q->tail + 1) % BUS_RING_CAPACITY;
    q->count++;
}

int bus_send(Bus* bus, const Frame* f) {
    (void)bus;
    for (uint8_t i = 0; i < g_max_nodes; ++i) {
        enqueue(&g_queues[i], f);
    }
    return 1;
}

int bus_recv(Bus* bus, Frame* out, uint16_t timeout_ms) {
    Queue* q = &g_queues[bus->index];
    uint32_t start = hal_millis();
    while (q->count == 0) {
        if (timeout_ms && (hal_millis() - start >= timeout_ms)) return 0;
        hal_delay(1);
    }
    *out = q->buffer[q->head];
    q->head = (q->head + 1) % BUS_RING_CAPACITY;
    q->count--;
    return 1;
}
