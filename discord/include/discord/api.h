// Copyright 2025 JesusTouchMe

#ifndef DISCORD_API_H
#define DISCORD_API_H 1

#include "utils/webutils.h"

int DiscordAPI_Init(void);
void DiscordAPI_Shutdown(void);

void DiscordAPI_SetAuth(const char* auth);

HTTPResponse* DiscordAPI_SendRequest(Arena* arena, const char* method, const char* path, const char* body);

#endif //DISCORD_API_H