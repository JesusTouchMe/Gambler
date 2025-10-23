// Copyright 2025 JesusTouchMe

#include "discord.h"

#include <inttypes.h>

static void CreatePath(char* out, size_t out_size, snowflake_t channel_id) {
    snprintf(out, out_size, "/api/v10/channels/%" PRIu64 "/messages", channel_id);
}

void InitDefaultMessageContent(MessageContent* message) {
    memset(message, 0, sizeof(MessageContent));
    message->content = "";
}

static void EncodeMessageContent(char* req, size_t req_size, int* offset, const MessageReference* reference) {
    switch (reference->type.state) {
        case OPTION_ABSENT:
            break;
        case OPTION_NULL:
            *offset += snprintf(req + *offset, req_size - *offset, "\"type\":null");
            break;
        case OPTION_EXISTS:
            *offset += snprintf(req + *offset, req_size - *offset, "\"type\":%" PRIu64, reference->type.value);
            break;
    }

    switch (reference->message_id.state) {
        case OPTION_ABSENT:
            break;
        case OPTION_NULL:
            *offset += snprintf(req + *offset, req_size - *offset, "\"message_id\":null");
            break;
        case OPTION_EXISTS:
            *offset += snprintf(req + *offset, req_size - *offset, "\"message_id\":\"%" PRIu64 "\"", reference->message_id.value);
            break;
    }

    switch (reference->channel_id.state) {
        case OPTION_ABSENT:
            break;
        case OPTION_NULL:
            *offset += snprintf(req + *offset, req_size - *offset, "\"channel_id\":null");
            break;
        case OPTION_EXISTS:
            *offset += snprintf(req + *offset, req_size - *offset, "\"channel_id\":\"%" PRIu64 "\"", reference->channel_id.value);
            break;
    }

    switch (reference->guild_id.state) {
        case OPTION_ABSENT:
            break;
        case OPTION_NULL:
            *offset += snprintf(req + *offset, req_size - *offset, "\"guild_id\":null");
            break;
        case OPTION_EXISTS:
            *offset += snprintf(req + *offset, req_size - *offset, "\"guild_id\":\"%" PRIu64 "\"", reference->guild_id.value);
            break;
    }

    switch (reference->fail_if_not_exists.state) {
        case OPTION_ABSENT:
            break;
        case OPTION_NULL:
            *offset += snprintf(req + *offset, req_size - *offset, "\"fail_if_not_exists\":null");
            break;
        case OPTION_EXISTS:
            *offset += snprintf(req + *offset, req_size - *offset, "\"fail_if_not_exists\":%s", BOOLEAN_TO_STRING(reference->fail_if_not_exists.value));
            break;
    }
}

int SendMessageEx(snowflake_t channel_id, const MessageContent* message) {
    char path[256];
    char req[1024];
    int offset = 0;

    offset += snprintf(req + offset, sizeof(req) - offset, "{\"content\":\"%s\"", message->content);
    switch (message->nonce.state) {
        case OPTION_ABSENT:
            break;
        case OPTION_NULL:
            offset += snprintf(req + offset, sizeof(req) - offset, ",\"nonce\":null");
            break;
        case OPTION_EXISTS:
            if (message->nonce.value.is_string) {
                offset += snprintf(req + offset, sizeof(req) - offset, ",\"nonce\":\"%s\"", message->nonce.value.string);
            } else {
                offset += snprintf(req + offset, sizeof(req) - offset, ",\"nonce\":% " PRIu64 "", message->nonce.value.integer);
            }
            break;
    }

    offset += snprintf(req + offset, sizeof(req) - offset, ",\"tts\":%s", BOOLEAN_TO_STRING(message->tts));

    switch (message->message_reference.state) {
        case OPTION_ABSENT:
            break;
        case OPTION_NULL:
            offset += snprintf(req + offset, sizeof(req) - offset, ",\"message_reference\":null");
            break;
        case OPTION_EXISTS:
            offset += snprintf(req + offset, sizeof(req) - offset, ",\"message_reference\":{");
            EncodeMessageContent(req, sizeof(req), &offset, &message->message_reference.value);
            offset += snprintf(req + offset, sizeof(req) - offset, "}");
            break;
    }

    switch (message->enforce_nonce.state) {
        case OPTION_ABSENT:
            break;
        case OPTION_NULL:
            offset += snprintf(req + offset, sizeof(req) - offset, ",\"enforce_nonce\":null");
            break;
        case OPTION_EXISTS:
            offset += snprintf(req + offset, sizeof(req) - offset, ",\"enforce_nonce\":%s", BOOLEAN_TO_STRING(message->enforce_nonce.value));
            break;
    }

    offset += snprintf(req + offset, sizeof(req) - offset, "}");

    CreatePath(path, sizeof(path), channel_id);

    HTTPResponse* res = DiscordAPI_SendRequest(Discord_GetEventArena(), "POST", path, req);
    if (res == NULL || res->code != 200) {
        if (res != NULL) {
            printf("SendMessageEx error: code=%d, body:\n", res->code);
            printf("%s\n", res->body);
        } else {
            printf("SendMessageEx error: web problem\n");
        }

        fflush(stdout);

        return 1;
    }

    return 0;
}

int SendMessage(snowflake_t channel_id, const char* message) {
    MessageContent message_content;
    InitDefaultMessageContent(&message_content);
    if (message != NULL) message_content.content = message;
    return SendMessageEx(channel_id, &message_content);
}

int SendReply(snowflake_t channel_id, snowflake_t message_id, const char* message) {
    MessageContent message_content;
    InitDefaultMessageContent(&message_content);
    if (message != NULL) message_content.content = message;
    message_content.message_reference.state = OPTION_EXISTS;
    ASSIGN_OPTIONAL(message_content.message_reference.value.message_id, message_id);
    return SendMessageEx(channel_id, &message_content);
}
