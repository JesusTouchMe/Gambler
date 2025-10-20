// Copyright 2025 JesusTouchMe

#include "internal/memory.h"

#include "utils/webutils.h"

#include "discord.h"

#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>

static SSL_CTX* g_ssl_ctx = NULL;
static WSClient g_ws_client;

static char g_token[256];
static long long g_intents = 0; // TODO: some defaults

static char g_session_id[64];
static char g_gateway_resume_host[128];
static char g_gateway_resume_port[8] = "443";

static volatile int g_heartbeat_interval = 0;
static volatile long long g_last_seq = 0;
static volatile bool g_running = false;

static Arena g_event_arena;

// event handlers
static DiscordReadyCallback g_on_ready = NULL;
static DiscordMessageCreateCallback g_on_message_create = NULL;

void Discord_LibInit(void) {
    SSL_library_init();
    OpenSSL_add_all_algorithms();
    SSL_load_error_strings();

    g_ssl_ctx = SSL_CTX_new(TLS_client_method());
    g_event_arena = ArenaCreate(0);
}

void Discord_SetToken(const char* token) {
    strncpy(g_token, token, sizeof(g_token) - 1);
    g_token[sizeof(g_token) - 1] = '\0';
}

void Discord_AddIntent(long long intent) {
    g_intents |= intent;
}

void Discord_RemoveIntent(long long intent) {
    g_intents &= ~intent;
}

void Discord_OnReady(DiscordReadyCallback callback) {
    g_on_ready = callback;
}

void Discord_OnMessageCreate(DiscordMessageCreateCallback callback) {
    g_on_message_create = callback;
}

static void ConnectGateway() {
    if (WS_Connect(&g_ws_client, g_ssl_ctx, GATEWAY_HOST, GATEWAY_PORT, "/?v=10&encoding=json") != 0) {
        g_running = false;
        return;
    }
    g_running = true;
}

static void ResumeGateway() {
    if (WS_Connect(&g_ws_client, g_ssl_ctx, GATEWAY_HOST, GATEWAY_PORT, "/?v=10&encoding=json") != 0) {
        g_running = false;
        return;
    }

    char payload[1024];
    snprintf(payload, sizeof(payload), "{\"op\":6,\"d\":{\"token\":\"%s\",\"session_id\":\"%s\",\"seq\":%lld}}", g_token, g_session_id, g_last_seq);

    g_running = true;
}

static void DisconnectGateway(int code) {
    WS_Disconnect(g_ws_client, code);
    g_running = false;
}

static void SendHeartbeat() {
    char payload[128];
    snprintf(payload, sizeof(payload), "{\"op\":1,\"d\":%lld} ", g_last_seq);
    WS_SendText(&g_ws_client, payload);
}

void* HeartbeatThread(void* arg) {
    (void) arg;

    while (g_running && g_heartbeat_interval > 0) {
        usleep(g_heartbeat_interval * 1000);
        SendHeartbeat();
    }

    return NULL;
}

static void HandleEvent(const char* json, jsmntok_t* tokens, size_t token_count, JsonObject t, JsonObject d) {
    if (t == JSON_NULL || d == JSON_NULL || tokens[t].type != JSMN_STRING) {
        DisconnectGateway(UNSUPPORTED_DATA);
        return;
    }

    if (jsoneq(json, tokens[t], "READY")) {
        if (tokens[d].type != JSMN_OBJECT) {
            DisconnectGateway(UNSUPPORTED_DATA);
            return;
        }

        JsonObject session_id = jsmn_find_key(json, tokens, d, "session_id");
        JsonObject resume_gateway_url = jsmn_find_key(json, tokens, d, "resume_gateway_url");

        if (session_id == JSON_NULL || resume_gateway_url == JSON_NULL) {
            DisconnectGateway(UNSUPPORTED_DATA);
            return;
        }

        if (tokens[session_id].type != JSMN_STRING || tokens[resume_gateway_url].type != JSMN_STRING) {
            DisconnectGateway(UNSUPPORTED_DATA);
            return;
        }

        jsmn_copy_string(json, tokens, session_id, g_session_id, sizeof(g_session_id));

        char url_container[2048];
        jsmn_copy_string(json, tokens, resume_gateway_url, url_container, sizeof(url_container));

        const char* url = url_container;

        if (strncmp(url, "wss://", 6) == 0) {
            url += 6;
            strcpy(g_gateway_resume_port, "433");
        } else {
            url += 5;
            strcpy(g_gateway_resume_port, "80");
        }

        const char* slash = strchr(url, '/');
        if (slash != NULL) {
            size_t host_len = slash - url;
            if (host_len >= sizeof(g_gateway_resume_host)) {
                printf("host_len is bigger than g_gateway_resume_host. this should IMMEDIATELY be reported and fixed!\n");
                DisconnectGateway(INTERNAL_ERROR);
                return;
            }

            strncpy(g_gateway_resume_host, url, host_len);
            g_gateway_resume_host[host_len] = '\0';
        } else {
            strncpy(g_gateway_resume_host, url, sizeof(g_gateway_resume_host) - 1);
            g_gateway_resume_host[sizeof(g_gateway_resume_host) - 1] = '\0';
        }

        char *colon = strchr(g_gateway_resume_host, ':');
        if (colon) {
            *colon = '\0';
            strncpy(g_gateway_resume_port, colon + 1, sizeof(g_gateway_resume_port) - 1);
            g_gateway_resume_port[sizeof(g_gateway_resume_port) - 1] = '\0';
        }

        if (g_on_ready != NULL) g_on_ready();
    } else if (jsoneq(json, tokens[t], "MESSAGE_CREATE")) {
        JsonObject content = jsmn_find_key(json, tokens, d, "content");
        if (content != JSON_NULL) {
            int len = tokens[content].end - tokens[content].start;
            char* s = ArenaAlloc(&g_event_arena, len + 1);
            strncpy(s, json + tokens[content].start, len);
            s[len] = '\0';

            if (g_on_message_create != NULL) g_on_message_create(s);
        }
    }
}

static void HandleGatewayEvent(const char* json) {
    jsmn_parser parser;
    jsmn_init(&parser);

    int token_count = jsmn_parse(&parser, json, strlen(json), NULL, 0);
    jsmntok_t* tokens = ArenaAlloc(&g_event_arena, token_count * sizeof(jsmntok_t));
    jsmn_init(&parser);
    int err = jsmn_parse(&parser, json, strlen(json), tokens, token_count);

    if (err < 0) {
        printf("json error: %d\n", err);
        return;
    } else if (err != token_count) {
        printf("json token count mismatch: %d != %d\n", err, token_count);
        return;
    }

    int op = -1;
    JsonObject t = JSON_NULL;
    JsonObject d = JSON_NULL;

    // don't use the jsmn extensions from jsonutils because this is faster
    int i = 1;
    for (int k = 0; k < tokens[0].size; k++) {
        if (tokens[i].type == JSMN_STRING) {
            int len = tokens[i].end - tokens[i].start;
            const char* key = json + tokens[i].start;

            if (len == 2 && strncmp(key, "op", 2) == 0) {
                jsmntok_t v = tokens[i + 1];
                op = atoi(json + v.start);
            } else if (len == 1 && *key == 't') {
                t = i + 1;
            } else if (len == 1 && *key == 'd') {
                d = i + 1;
            } else if (len == 1 && *key == 's') {
                jsmntok_t v = tokens[i + 1];
                g_last_seq = atoi(json + v.start);
            }
        }

        i += jsmn_skip(&tokens[i + 1]) + 2;
    }

    if (op == 0) {
        HandleEvent(json, tokens, token_count, t, d);
    } else if (op == 1) {
        SendHeartbeat();
    } else if (op == 7) {
        DisconnectGateway(DONT_SEND_CODE);
        ConnectGateway();
    } else if (op == 9) {
        if (d == JSON_NULL || tokens[d].type != JSMN_PRIMITIVE || json[tokens[d].start] != 't' || json[tokens[d].start] != 'f') {
            DisconnectGateway(UNSUPPORTED_DATA);
            return;
        }

        if (json[tokens[d].start] == 't') { // true, we can resume
            DisconnectGateway(DONT_SEND_CODE);
            ResumeGateway();
        } else {
            DisconnectGateway(GOING_AWAY);
        }
    } else if (op == 10) {
        JsonObject heartbeat_interval = jsmn_find_key(json, tokens, d, "heartbeat_interval");
        g_heartbeat_interval = atoi(json + tokens[heartbeat_interval].start);

        pthread_t heartbeat_thread;
        pthread_create(&heartbeat_thread, NULL, HeartbeatThread, NULL);
        pthread_detach(heartbeat_thread);

        usleep(500 * 1000);

        char identify[1024];
        snprintf(identify, sizeof(identify),
                 "{\"op\":2,\"d\":{\"token\":\"%s\",\"intents\":%lld,\"properties\":{\"os\":\"linux\",\"browser\":\"gambler\",\"device\":\"gambler\"},\"compress\":false,\"presence\":{\"status\":\"online\"}}}",
                 g_token, g_intents);

        WS_SendText(&g_ws_client, identify);
    }
}

void Discord_Run(void) {
    ConnectGateway();

    while (g_running) {
        ArenaReset(&g_event_arena);

        char* payload;
        int n = WS_RecvText(&g_ws_client, &payload, &g_event_arena);
        if (n > 0) {
            printf("[WS] %s\n", payload);
            fflush(stdout);
            HandleGatewayEvent(payload);
        } else if (n == -1) {
            switch (g_ws_client.last_close_code) { // can we resume?
                case 4000:
                case 4001:
                case 4002:
                case 4003:
                case 4005:
                case 4007:
                case 4008:
                case 4009: {
                    DisconnectGateway(DONT_SEND_CODE);
                    ResumeGateway();
                    break;
                }

                default: {
                    printf("Connection closed: code=%d\n", g_ws_client.last_close_code);
                    fflush(stdout);
                    goto outtahere;
                }
            }
        }
    }

    outtahere:

    DisconnectGateway(GOING_AWAY);
}
