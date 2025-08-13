#include "bus.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>

#define BUS_RING_CAPACITY 64

typedef struct {
    Frame buffer[BUS_RING_CAPACITY];
    size_t head;
    size_t tail;
    size_t count;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    uint8_t node_index;
} RingQueue;

struct BusSubscriber {
    RingQueue* queue;
};

typedef struct {
    size_t max_subscribers;
    size_t num_subscribers;
    RingQueue* rings; // array of ring queues
    pthread_mutex_t global_mutex;
} BusState;

static BusState g_bus = {0};

static int ring_push(RingQueue* rq, const Frame* f) {
    if (rq->count == BUS_RING_CAPACITY) {
        // drop oldest
        rq->head = (rq->head + 1) % BUS_RING_CAPACITY;
        rq->count--;
    }
    rq->buffer[rq->tail] = *f;
    rq->tail = (rq->tail + 1) % BUS_RING_CAPACITY;
    rq->count++;
    return 0;
}

static int ring_pop(RingQueue* rq, Frame* out) {
    if (rq->count == 0) return -1;
    *out = rq->buffer[rq->head];
    rq->head = (rq->head + 1) % BUS_RING_CAPACITY;
    rq->count--;
    return 0;
}

int bus_init(size_t max_subscribers) {
    if (g_bus.rings) return 0; // already inited
    g_bus.max_subscribers = max_subscribers ? max_subscribers : 8;
    g_bus.num_subscribers = 0;
    g_bus.rings = (RingQueue*)calloc(g_bus.max_subscribers, sizeof(RingQueue));
    if (!g_bus.rings) return -1;
    pthread_mutex_init(&g_bus.global_mutex, NULL);
    for (size_t i = 0; i < g_bus.max_subscribers; ++i) {
        RingQueue* rq = &g_bus.rings[i];
        rq->head = rq->tail = rq->count = 0;
        pthread_mutex_init(&rq->mutex, NULL);
        pthread_cond_init(&rq->cond, NULL);
        rq->node_index = (uint8_t)i;
    }
    return 0;
}

void bus_shutdown(void) {
    if (!g_bus.rings) return;
    for (size_t i = 0; i < g_bus.max_subscribers; ++i) {
        pthread_mutex_destroy(&g_bus.rings[i].mutex);
        pthread_cond_destroy(&g_bus.rings[i].cond);
    }
    pthread_mutex_destroy(&g_bus.global_mutex);
    free(g_bus.rings);
    memset(&g_bus, 0, sizeof(g_bus));
}

BusSubscriber* bus_subscribe(uint8_t node_index) {
    pthread_mutex_lock(&g_bus.global_mutex);
    if (g_bus.num_subscribers >= g_bus.max_subscribers) {
        pthread_mutex_unlock(&g_bus.global_mutex);
        return NULL;
    }
    size_t slot = g_bus.num_subscribers++;
    RingQueue* rq = &g_bus.rings[slot];
    rq->node_index = node_index;
    BusSubscriber* sub = (BusSubscriber*)malloc(sizeof(BusSubscriber));
    if (!sub) {
        pthread_mutex_unlock(&g_bus.global_mutex);
        return NULL;
    }
    sub->queue = rq;
    pthread_mutex_unlock(&g_bus.global_mutex);
    return sub;
}

int bus_broadcast(const Frame* frame) {
    // deliver to all subscribers
    for (size_t i = 0; i < g_bus.num_subscribers; ++i) {
        RingQueue* rq = &g_bus.rings[i];
        pthread_mutex_lock(&rq->mutex);
        ring_push(rq, frame);
        pthread_cond_signal(&rq->cond);
        pthread_mutex_unlock(&rq->mutex);
    }
    return 0;
}

static int cond_timedwait_ms(pthread_cond_t* cond, pthread_mutex_t* mutex, uint32_t timeout_ms) {
    if (timeout_ms == (uint32_t)-1) {
        return pthread_cond_wait(cond, mutex);
    }
    if (timeout_ms == 0) return ETIMEDOUT;

    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += timeout_ms / 1000;
    ts.tv_nsec += (long)(timeout_ms % 1000) * 1000000L;
    if (ts.tv_nsec >= 1000000000L) {
        ts.tv_sec += 1;
        ts.tv_nsec -= 1000000000L;
    }
    return pthread_cond_timedwait(cond, mutex, &ts);
}

int bus_recv(BusSubscriber* sub, Frame* out, uint32_t timeout_ms) {
    if (!sub || !out) return -1;
    RingQueue* rq = sub->queue;
    pthread_mutex_lock(&rq->mutex);
    if (rq->count == 0) {
        int rc = cond_timedwait_ms(&rq->cond, &rq->mutex, timeout_ms);
        if (rc == ETIMEDOUT) {
            pthread_mutex_unlock(&rq->mutex);
            return 1; // timeout
        }
    }
    int popped = ring_pop(rq, out);
    pthread_mutex_unlock(&rq->mutex);
    return popped == 0 ? 0 : 1; // 0=ok,1=no data
}
