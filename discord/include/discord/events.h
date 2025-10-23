// Copyright 2025 JesusTouchMe

#ifndef DISCORD_EVENTS_H
#define DISCORD_EVENTS_H 1

#include "utils/jsonutils.h"

typedef struct Event {
    const char* json;
    const jsmntok_t* tokens;
    JsonObject t;
    JsonObject d;

    struct Event* next;
} Event;

void EventLoop_Init(int thread_count); // if thread_count <= 0, it will use all
void EventLoop_Shutdown(bool join);

void EventLoop_Enqueue(Event* event); // takes ownership of event

#endif //DISCORD_EVENTS_H