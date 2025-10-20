// Copyright 2025 JesusTouchMe

#include "discord.h"

#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>

#define CALL_OR_DEFAULT(func, d, ...) func == NULL ? d : func(__VA_ARGS__)

#ifndef GAMBLER_NOMAIN
int main(int argc, char** argv) {
    Discord_LibInit();

    const char* token = getenv("GAMBLER_TOKEN");
    if (token == NULL) {
        printf("GAMBLER_TOKEN env not set\n");
        fflush(stdout);
        return 1;
    }

    Discord_SetToken(token);

    void* dl = dlopen(NULL, RTLD_NOW);
    if (dl == NULL) return 1;

    DiscordGetIntents get_intents = dlsym(dl, "GetIntents");
    Discord_AddIntent(CALL_OR_DEFAULT(get_intents, 0));

    Discord_OnReady(dlsym(dl, "OnReady"));
    Discord_OnMessageCreate(dlsym(dl, "OnMessageCreate"));

    Discord_Run();

    dlclose(dl);
}
#endif