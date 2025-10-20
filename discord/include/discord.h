// Copyright 2025 JesusTouchMe

#ifndef DISCORD_H
#define DISCORD_H 1

#include "utils/jsonutils.h"

#include "config.h"
#include "intents.h"

typedef void (*DiscordReadyCallback)(void);
typedef void (*DiscordMessageCreateCallback)(const char* content);

void Discord_LibInit(void);

void Discord_SetToken(const char* token);
void Discord_AddIntent(long long intent);
void Discord_RemoveIntent(long long intent);

void Discord_OnReady(DiscordReadyCallback callback);
void Discord_OnMessageCreate(DiscordMessageCreateCallback callback); // TODO: message struct

void Discord_Run(void);

#endif // DISCORD_H
