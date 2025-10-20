// Copyright 2025 JesusTouchMe

#ifndef DISCORD_TYPES_H
#define DISCORD_TYPES_H 1

#include <stdint.h>

typedef uint64_t snowflake_t;

typedef struct Message {
    snowflake_t id;
    snowflake_t channel_id;
    const char* content;
} Message;

#endif // DISCORD_TYPES_H
