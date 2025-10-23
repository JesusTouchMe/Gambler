// Copyright 2025 JesusTouchMe

#ifndef DISCORD_H
#define DISCORD_H 1

#include "discord/api.h"
#include "discord/events.h"
#include "discord/function_types.h"
#include "discord/intents.h"
#include "discord/message.h"
#include "discord/types.h"

#include "utils/jsonutils.h"

void Discord_LibInit(void);
void Discord_LibShutdown(void);

void Discord_SetToken(const char* token);
void Discord_SetIntents(intents_t intents);
void Discord_AddIntent(intents_t intent);
void Discord_RemoveIntent(intents_t intent);

OnReadyFn Discord_OnReady(void);
OnMessageCreateFn Discord_OnmessageCreate(void);

void Discord_SetOnReady(OnReadyFn callback);
void Discord_SetOnMessageCreate(OnMessageCreateFn callback); // TODO: message struct

Arena* Discord_GetEventArena(void);

void Discord_Run(void);

#endif // DISCORD_H
