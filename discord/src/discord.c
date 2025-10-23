// Copyright 2025 JesusTouchMe

#include "internal/memory.h"

#include "utils/time.h"
#include "utils/webutils.h"

#include "discord.h"

#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>

SSL_CTX* g_ssl_ctx = NULL;
static char g_gateway_host[128];
static char g_gateway_port[8] = "443";
static WSClient g_ws_client;

static char g_token[256];
static intents_t g_intents = 0;

static char g_session_id[64];
static char g_gateway_resume_host[128];
static char g_gateway_resume_port[8] = "443";

static bool g_send_heartbeats = false;
static int g_heartbeat_interval = 0;
static long long g_last_seq = 0;
static volatile bool g_running = false;

static Arena g_event_arena;

// event handlers
static OnReadyFn g_on_ready = NULL;
static OnMessageCreateFn g_on_message_create = NULL;

static void DisconnectGateway(int code);

void Discord_LibInit(void) {
    SSL_library_init();
    OpenSSL_add_all_algorithms();
    SSL_load_error_strings();

    g_ssl_ctx = SSL_CTX_new(TLS_client_method());
    g_event_arena = ArenaCreate(0);

    if (DiscordAPI_Init() != 0) {
        exit(1);
    }

    HTTPResponse* res = DiscordAPI_SendRequest(&g_event_arena, "GET", "/api/v10/gateway", "");
    if (res == NULL) {
        printf("Cannot get gateway url from API\n");
        exit(1);
    }

    const char* json = res->body;

    jsmn_parser parser;
    jsmn_init(&parser);

    int token_count = jsmn_parse(&parser, json, strlen(json), NULL, 0);
    jsmntok_t* tokens = ArenaAlloc(&g_event_arena, token_count * sizeof(jsmntok_t));
    jsmn_init(&parser);
    int err = jsmn_parse(&parser, json, strlen(json), tokens, token_count);

    if (err < 0) {
        printf("json error: %d\n", err);
        exit(1);
    } else if (err != token_count) {
        printf("json token count mismatch: %d != %d\n", err, token_count);
        exit(1);
    }

    if (tokens[0].type != JSMN_OBJECT) {
        printf("bad response from /gateway (response is not object)\n");
        exit(1);
    }

    JsonObject url_obj = jsmn_find_key(json, tokens, 0, "url");
    if (url_obj == JSON_NULL || tokens[url_obj].type != JSMN_STRING) {
        printf("bad response from /gateway (url is not string)\n");
        exit(1);
    }

    char url_container[2048];
    jsmn_copy_string(json, tokens, url_obj, url_container, sizeof(url_container));

    const char* url = url_container;

    if (strncmp(url, "wss://", 6) == 0) {
        url += 6;
        strcpy(g_gateway_port, "443");
    } else {
        url += 5;
        strcpy(g_gateway_port, "80");
    }

    const char* slash = strchr(url, '/');
    if (slash != NULL) {
        size_t host_len = slash - url;
        if (host_len >= sizeof(g_gateway_host)) {
            printf("host_len is bigger than g_gateway_host. this should IMMEDIATELY be reported and fixed!\n");
            exit(1);
        }

        strncpy(g_gateway_host, url, host_len);
        g_gateway_host[host_len] = '\0';
    } else {
        strncpy(g_gateway_host, url, sizeof(g_gateway_host) - 1);
        g_gateway_host[strlen(url)] = '\0';
    }

    char* colon = strchr(g_gateway_host, ':');
    if (colon) {
        *colon = '\0';
        strncpy(g_gateway_port, colon + 1, sizeof(g_gateway_port) - 1);
        g_gateway_port[sizeof(g_gateway_port) - 1] = '\0';
    }
}

void Discord_LibShutdown(void) {
    if (g_running) {
        DisconnectGateway(GOING_AWAY);
    }

    DiscordAPI_Shutdown();

    SSL_CTX_free(g_ssl_ctx);
    ArenaDestroy(g_event_arena);

    EVP_cleanup();
    ERR_free_strings();
    CRYPTO_cleanup_all_ex_data();
    CONF_modules_unload(1);
}

void Discord_SetToken(const char* token) {
    strncpy(g_token, token, sizeof(g_token) - 1);
    g_token[sizeof(g_token) - 1] = '\0';

    char auth[260];
    snprintf(auth, 260, "Bot %s", token);

    DiscordAPI_SetAuth(auth);
}

void Discord_SetIntents(intents_t intents) {
    g_intents = intents;
}

void Discord_AddIntent(intents_t intent) {
    g_intents |= intent;
}

void Discord_RemoveIntent(intents_t intent) {
    g_intents &= ~intent;
}

OnReadyFn Discord_OnReady(void) {
    return g_on_ready;
}

OnMessageCreateFn Discord_OnmessageCreate(void) {
    return g_on_message_create;
}

void Discord_SetOnReady(OnReadyFn callback) {
    g_on_ready = callback;
}

void Discord_SetOnMessageCreate(OnMessageCreateFn callback) {
    g_on_message_create = callback;
}

static void ConnectGateway() {
    if (WS_Connect(&g_ws_client, g_ssl_ctx, g_gateway_host, g_gateway_port, "/?v=10&encoding=json") != 0) {
        g_running = false;
        return;
    }
    g_running = true;
}

static void ResumeGateway() {
    if (WS_Connect(&g_ws_client, g_ssl_ctx, g_gateway_resume_host, g_gateway_resume_port, "/?v=10&encoding=json") != 0) {
        g_running = false;
        return;
    }

    char payload[1024];
    snprintf(payload, sizeof(payload), "{\"op\":6,\"d\":{\"token\":\"%s\",\"session_id\":\"%s\",\"seq\":%lld}}", g_token, g_session_id, g_last_seq);

    WS_SendText(&g_ws_client, payload);

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

static void HandleEvent(Event* event) {
    const char* json = event->json;
    const jsmntok_t* tokens = event->tokens;
    JsonObject t = event->t;
    JsonObject d = event->d;

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
    }

    EventLoop_Enqueue(event);
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

        int val = i + 1;
        int skip = jsmn_subtree_size(tokens, val);
        i = val + 1 + skip;
    }

    if (op == 0) {
        size_t json_len = strlen(json);

        char* raw = HeapAlloc(sizeof(Event) + token_count * sizeof(jsmntok_t) + json_len + 1);

        Event* event = (Event*) raw;
        jsmntok_t* event_tokens = (jsmntok_t*) (raw + sizeof(Event));
        char* event_json = raw + sizeof(Event) + (token_count * sizeof(jsmntok_t));

        memcpy(event_tokens, tokens, token_count * sizeof(jsmntok_t));
        memcpy(event_json, json, json_len);
        event_json[json_len] = '\0';

        event->json = event_json;
        event->tokens = event_tokens;
        event->t = t;
        event->d = d;

        HandleEvent(event);
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
        g_send_heartbeats = true;

        char identify[1024];
        snprintf(identify, sizeof(identify),
                 "{\"op\":2,\"d\":{\"token\":\"%s\",\"intents\":%lu,\"properties\":{\"os\":\"linux\",\"browser\":\"gambler\",\"device\":\"gambler\"},\"compress\":false,\"presence\":{\"status\":\"online\"}}}",
                 g_token, g_intents);

        WS_SendText(&g_ws_client, identify);
    }
}

Arena* Discord_GetEventArena(void) {
    return &g_event_arena;
}

void Discord_Run(void) {
    ConnectGateway();

    uint64_t next_heartbeat = 0;

    while (g_running) {
        ArenaReset(&g_event_arena);

        int timeout_ms = 100;
        if (g_send_heartbeats) {
            uint64_t now = NowMs();
            if (next_heartbeat > now) timeout_ms = next_heartbeat - now;
            else timeout_ms = 0;
        }

        struct timeval tv = {timeout_ms / 1000, (timeout_ms % 1000) * 1000};
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(g_ws_client.sock, &fds);

        int ret = select(g_ws_client.sock + 1, &fds, NULL, NULL, &tv);
        if (ret > 0 && FD_ISSET(g_ws_client.sock, &fds)) {
            char* payload;
            int n = WS_RecvText(&g_ws_client, &payload, &g_event_arena);
            if (n > 0) {
                printf("[WS] %s\n", payload);
                fflush(stdout);
                HandleGatewayEvent(payload);
            } else if (n == -1) {
                printf("[WS] Error. last_close_code=%d\n", g_ws_client.last_close_code);

                switch (g_ws_client.last_close_code) {
                    // can we resume?
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
                        DisconnectGateway(DONT_SEND_CODE);
                        ConnectGateway();
                        break;
                    }
                }
            }
        }

        if (g_send_heartbeats) {
            uint64_t now = NowMs();
            if (now >= next_heartbeat) {
                SendHeartbeat();
                do {
                    next_heartbeat += g_heartbeat_interval;
                } while (next_heartbeat <= now);
            }
        }
    }

    outtahere:

    DisconnectGateway(GOING_AWAY);
}
