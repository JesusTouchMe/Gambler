// Copyright 2025 JesusTouchMe

#ifndef DISCORD_FUNCTION_TYPES_H
#define DISCORD_FUNCTION_TYPES_H 1

#include "discord/intents.h"
#include "discord/types.h"

typedef int (*ProgramMainFn)(int argc, const char** argv);
typedef void (*ProgramExitFn)(void);

typedef const char* (*GetTokenFn)(void);
typedef intents_t (*GetIntentsFn)(void);

typedef void (*OnReadyFn)(void);
typedef void (*OnMessageCreateFn)(const Message* message);

#endif //DISCORD_FUNCTION_TYPES_H