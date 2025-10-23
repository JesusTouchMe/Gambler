// Copyright 2025 JesusTouchMe

#include "discord/types.h"

#include <stdlib.h>

static int ParseInteger(integer_t* i, const char* json, const jsmntok_t* tokens, JsonObject object, const char* key) {
    JsonObject obj = jsmn_find_key(json, tokens, object, key);
    if (obj == JSON_NULL) return 1;
    char c = json[tokens[obj].start];
    if (tokens[obj].type != JSMN_PRIMITIVE && (c < '0' || c > '9')) return 1;
    *i = strtoull(json + tokens[obj].start, NULL, 10);
    return 0;
}

static int ParseSnowflake(snowflake_t* i, const char* json, const jsmntok_t* tokens, JsonObject object, const char* key) {
    JsonObject obj = jsmn_find_key(json, tokens, object, key);
    if (obj == JSON_NULL) return 1;
    if (tokens[obj].type != JSMN_STRING) return 1;
    *i = strtoull(json + tokens[obj].start, NULL, 10);
    return 0;
}

static int ParseString(string_t* s, const char* json, const jsmntok_t* tokens, JsonObject object, const char* key, Arena* arena) {
    JsonObject obj = jsmn_find_key(json, tokens, object, key);
    if (obj == JSON_NULL) return 1;
    if (tokens[obj].type != JSMN_STRING) return 1;
    int len = tokens[obj].end - tokens[obj].start + 1;
    char* str = ArenaAlloc(arena, len);
    jsmn_copy_string(json, tokens, obj, str, len);
    *s = str;
    return 0;
}

int ParseMessage(Message* message, Arena* arena, const char* json, const jsmntok_t* tokens, JsonObject message_obj) {
    if (message_obj == JSON_NULL || tokens[message_obj].type != JSMN_OBJECT) return 1;

    if (ParseSnowflake(&message->id, json, tokens, message_obj, "id") != 0) return 1;
    if (ParseSnowflake(&message->channel_id, json, tokens, message_obj, "channel_id") != 0) return 1;
    if (ParseString(&message->content, json, tokens, message_obj, "content", arena) != 0) return 1;

    return 0;
}
