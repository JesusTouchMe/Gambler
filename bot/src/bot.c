// Copyright 2025 JesusTouchMe

#include "discord.h"

#include <stdio.h>

intents_t GetIntents(void) {
    return GUILD_MESSAGES | MESSAGE_CONTENT;
}

void OnReady(void) {
    printf("we ready cuh\n");
    fflush(stdout);
}

void OnMessageCreate(const Message* message) {
    printf("message: %s\n", message->content);
    fflush(stdout);
}