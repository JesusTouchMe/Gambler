// Copyright 2025 JesusTouchMe

#include "discord.h"

#define _GNU_SOURCE

#include <stdio.h>
#include <string.h>

intents_t GetIntents(void) {
    return GUILD_MESSAGES | MESSAGE_CONTENT;
}

void OnReady(void) {
    printf("we ready cuh\n");
    fflush(stdout);

    srand(time(NULL));
}

void OnMessageCreate(const Message* message) {
    static const char* voicelines[] = {
        "yo",
        "hello",
        "what up nga",
        "FUCK YOU KILL YOURSELF STUPID NIGGA FAGGOT",
        "yellow"
    };

    if (strcasestr(message->content, "hello") != NULL) {
        const char* voiceline = voicelines[rand() % (sizeof(voicelines) / sizeof(voicelines[0]))];

        if (SendReply(message->channel_id, message->id, voiceline) != 0) {
            printf("error sending message :(\n");
            fflush(stdout);
        }
    }
}