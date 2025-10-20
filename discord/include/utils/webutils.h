// Copyright 2025 JesusTouchMe

#ifndef DISCORD_UTILS_WEBUTILS_H
#define DISCORD_UTILS_WEBUTILS_H 1

#include "internal/memory.h"

#include <openssl/ssl.h>
#include <openssl/err.h>

#include <stdbool.h>

#define DONT_SEND_CODE -1
#define NORMAL_CLOSURE 1000
#define GOING_AWAY 1001
#define PROTOCOL_ERROR 1002
#define UNSUPPORTED_DATA 1003
#define NO_STATUS_RCVD 1005
#define ABNORMAL_CLOSURE 1006
#define INVALID_FRAME_PAYLOAD_DATA 1007
#define POLICY_VIOLATION 1008
#define MESSAGE_TOO_BIG 1009
#define MANDATORY_EXT 1010
#define INTERNAL_ERROR 1011
#define SERVICE_RESTART 1012
#define TRY_AGAIN_LATER 1013
#define BAD_GATEWAY 1014
#define TLS_HANDSHAKE 1015

typedef struct HTTPClient {
    int sock;
    SSL_CTX* ctx;
    SSL* ssl;
    char host[128];
    char port[8];
    bool connected;
    char authorization[256];
} HTTPClient;

typedef struct HTTPResponse {
    int code;
    char* body;
} HTTPResponse;

typedef struct WSClient {
    int sock;
    SSL* ssl;
    int last_close_code;
} WSClient;

int HTTP_Connect(HTTPClient* client, SSL_CTX* ctx, const char* host, const char* port);
void HTTP_Disconnect(HTTPClient* client);

void HTTP_SetAuthorization(HTTPClient* client, const char* authorization);

HTTPResponse* HTTP_Request(HTTPClient* client, Arena* arena, const char* method, const char* path, const char* body);

int WS_Connect(WSClient* client, SSL_CTX* ctx, const char* host, const char* port, const char* path);
void WS_Disconnect(WSClient client, int code);

int WS_SendText(WSClient* client, const char* text);
int WS_RecvText(WSClient* client, char** out_payload, Arena* arena);

#endif // DISCORD_UTILS_WEBUTILS_H
