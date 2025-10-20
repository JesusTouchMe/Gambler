// Copyright 2025 JesusTouchMe

#include "discord.h"
#include "intents.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void ReadyHandler(void) {
    printf("Bot ready\n");
    fflush(stdout);
}

void MessageCreateHandler(const Message* message) {
    printf("message: %s\n", message->content);
    fflush(stdout);
}

int main() {
    Discord_LibInit();

    const char* token = getenv("GAMBLER_TOKEN");
    if (token == NULL) {
        printf("GAMBLER_TOKEN env not set\n");
        fflush(stdout);
        return 1;
    }

    Discord_SetToken(token);

    Discord_AddIntent(GUILD_MESSAGES);
    Discord_AddIntent(MESSAGE_CONTENT);

    Discord_OnReady(ReadyHandler);
    Discord_OnMessageCreate(MessageCreateHandler);

    Discord_Run();

    return 0;
}