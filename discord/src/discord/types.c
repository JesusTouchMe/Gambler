// Copyright 2025 JesusTouchMe

#include "discord/types.h"

#include <stdlib.h>

static int ParseBoolean(boolean_t* b, const char* json, const jsmntok_t* tokens, JsonObject object, const char* key) {

}

int ParseMessage(Message* message, Arena* arena, const char* json, const jsmntok_t* tokens, JsonObject message_obj) {
    if (message_obj == JSON_NULL || tokens[message_obj].type != JSMN_OBJECT) return 1;

    JsonObject id = jsmn_find_key(json, tokens, message_obj, "id");
    if (id == JSON_NULL) return 1;
    message->id = strtoull(json + tokens[id].start, NULL, 10);

    JsonObject channel_id = jsmn_find_key(json, tokens, message_obj, "channel_id");
    if (channel_id == JSON_NULL) return 1;
    message->channel_id = strtoull(json + tokens[channel_id].start, NULL, 10);

    JsonObject content = jsmn_find_key(json, tokens, message_obj, "content");
    if (content == JSON_NULL) return 1;
    int content_len = tokens[content].end - tokens[content].start + 1;
    char* content_str = ArenaAlloc(arena, content_len);
    jsmn_copy_string(json, tokens, content, content_str, content_len);
    message->content = content_str;

    return 0;
}
