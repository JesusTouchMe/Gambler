// Copyright 2025 JesusTouchMe

#include "discord/events.h"

#include <pthread.h>

typedef struct EventLoop {
    pthread_t thread;

    bool active;

    pthread_mutex_t lock;
    pthread_cond_t cond;
    Event* front;
    Event* back;
} EventLoop;

static EventLoop g_event_loop;

void EventLoop_Init(int thread_count) {
    g_event_loop.front = NULL;
    g_event_loop.back = NULL;
}

void EventLoop_Shutdown(bool join) {
}

void EventLoop_Enqueue(Event* event) {
}
