// Copyright 2025 JesusTouchMe

#include "discord/api.h"

extern SSL_CTX* g_ssl_ctx;

static HTTPClient g_http_client;

int DiscordAPI_Init(void) {
    if (HTTP_Connect(&g_http_client, g_ssl_ctx, "discord.com", "443") != 0) return 1;

    return 0;
}

void DiscordAPI_Shutdown(void) {
    HTTP_Disconnect(&g_http_client);
}

void DiscordAPI_SetAuth(const char* auth) {
    HTTP_SetAuthorization(&g_http_client, auth);
}

HTTPResponse* DiscordAPI_SendRequest(Arena* arena, const char* method, const char* path, const char* body) {
    return HTTP_Request(&g_http_client, arena, method, path, body);
}
