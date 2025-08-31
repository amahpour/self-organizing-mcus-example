#define _POSIX_C_SOURCE 200809L
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "../../core/bus_interface.h"
#include "../../core/hal.h"

#define MAX_NODES 32
#define RING_CAPACITY 64

typedef struct {
    Frame buffer[RING_CAPACITY];
    size_t head, tail, count;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
} Queue;

struct Bus {
    uint8_t node_index;
    Queue* queue;
};

static Queue g_queues[MAX_NODES];
static size_t g_num_nodes = 0;
static pthread_mutex_t g_global_mutex = PTHREAD_MUTEX_INITIALIZER;

static int ring_push(Queue* q, const Frame* f) {
    if (q->count == RING_CAPACITY) {
        // Drop oldest frame
        q->head = (q->head + 1) % RING_CAPACITY;
        q->count--;
    }
    q->buffer[q->tail] = *f;
    q->tail = (q->tail + 1) % RING_CAPACITY;
    q->count++;
    return 0;
}

static int ring_pop(Queue* q, Frame* out) {
    if (q->count == 0)
        return -1;
    *out = q->buffer[q->head];
    q->head = (q->head + 1) % RING_CAPACITY;
    q->count--;
    return 0;
}

int bus_global_init(uint8_t max_nodes) {
    pthread_mutex_lock(&g_global_mutex);
    g_num_nodes = 0;
    for (size_t i = 0; i < MAX_NODES && i < max_nodes; ++i) {
        Queue* q = &g_queues[i];
        q->head = q->tail = q->count = 0;
        pthread_mutex_init(&q->mutex, NULL);
        pthread_cond_init(&q->cond, NULL);
    }
    pthread_mutex_unlock(&g_global_mutex);
    return 0;
}

void bus_global_shutdown(void) {
    pthread_mutex_lock(&g_global_mutex);
    for (size_t i = 0; i < g_num_nodes; ++i) {
        pthread_mutex_destroy(&g_queues[i].mutex);
        pthread_cond_destroy(&g_queues[i].cond);
    }
    g_num_nodes = 0;
    pthread_mutex_unlock(&g_global_mutex);
}

int bus_create(Bus** bus, uint8_t node_index, uint8_t rx_pin, uint8_t tx_pin) {
    (void) rx_pin;
    (void) tx_pin;  // Unused in simulation

    pthread_mutex_lock(&g_global_mutex);
    if (g_num_nodes >= MAX_NODES) {
        pthread_mutex_unlock(&g_global_mutex);
        return -1;
    }

    Bus* b = (Bus*) malloc(sizeof(Bus));
    if (!b) {
        pthread_mutex_unlock(&g_global_mutex);
        return -1;
    }

    b->node_index = node_index;
    b->queue = &g_queues[g_num_nodes++];
    *bus = b;

    pthread_mutex_unlock(&g_global_mutex);
    return 0;
}

void bus_destroy(Bus* bus) {
    if (bus) {
        free(bus);
    }
}

int bus_send(Bus* bus, const Frame* frame) {
    if (!bus || !frame)
        return -1;

    // Broadcast to all queues
    pthread_mutex_lock(&g_global_mutex);
    for (size_t i = 0; i < g_num_nodes; ++i) {
        Queue* q = &g_queues[i];
        pthread_mutex_lock(&q->mutex);
        ring_push(q, frame);
        pthread_cond_signal(&q->cond);
        pthread_mutex_unlock(&q->mutex);
    }
    pthread_mutex_unlock(&g_global_mutex);
    return 1;
}

int bus_recv(Bus* bus, Frame* frame, uint16_t timeout_ms) {
    if (!bus || !frame)
        return -1;

    Queue* q = bus->queue;
    pthread_mutex_lock(&q->mutex);

    if (q->count == 0) {
        if (timeout_ms == 0) {
            pthread_mutex_unlock(&q->mutex);
            return 0;  // No data, non-blocking
        }

        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_sec += timeout_ms / 1000;
        ts.tv_nsec += (long) (timeout_ms % 1000) * 1000000L;
        if (ts.tv_nsec >= 1000000000L) {
            ts.tv_sec += 1;
            ts.tv_nsec -= 1000000000L;
        }

        int rc = pthread_cond_timedwait(&q->cond, &q->mutex, &ts);
        if (rc != 0) {
            pthread_mutex_unlock(&q->mutex);
            return 0;  // Timeout
        }
    }

    int result = ring_pop(q, frame) == 0 ? 1 : 0;
    pthread_mutex_unlock(&q->mutex);
    return result;
}

