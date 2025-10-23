// Copyright 2025 JesusTouchMe

#include "discord.h"

#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>

#define CALL_OR_DEFAULT(func, d, ...) func == NULL ? d : func(__VA_ARGS__)

int main(int argc, char** argv) {
    void* dl = dlopen(NULL, RTLD_NOW);
    if (dl == NULL) return 1;

    ProgramMainFn program_main = dlsym(dl, "ProgramMain");
    if (program_main != NULL) {
        int code = program_main(argc, argv);
        if (code != 0) {
            dlclose(dl);
            return code;
        }
    }

    Discord_LibInit();

    GetTokenFn get_token = dlsym(dl, "GetToken");
    if (get_token != NULL) {
        Discord_SetToken(get_token());
    } else {
        const char* token = getenv("GAMBLER_TOKEN");
        if (token == NULL) {
            printf("GAMBLER_TOKEN env not set\n");
            dlclose(dl);
            return 1;
        }

        Discord_SetToken(token);
    }

    GetIntentsFn get_intents = dlsym(dl, "GetIntents");
    Discord_SetIntents(CALL_OR_DEFAULT(get_intents, 0));

    Discord_SetOnReady(dlsym(dl, "OnReady"));
    Discord_SetOnMessageCreate(dlsym(dl, "OnMessageCreate"));

    Discord_Run();

    ProgramExitFn program_exit = dlsym(dl, "ProgramExit");
    if (program_exit != NULL) {
        program_exit();
    }

    Discord_LibShutdown();

    dlclose(dl);

    return 0;
}