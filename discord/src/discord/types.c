// Copyright 2025 JesusTouchMe

#include "discord/types.h"

const Message* ParseMessage(Arena* arena, const char* json, jsmntok_t* tokens, JsonObject message_obj) {
    if (message_obj == JSON_NULL || tokens[message_obj].type != JSMN_OBJECT) return NULL;

    JsonObject content = jsmn_find_key(json, tokens, message_obj, "content");
    if (content == JSON_NULL) return NULL;

    int len = tokens[content].end - tokens[content].start + 1;

    char* content_str = ArenaAlloc(arena, len);
    jsmn_copy_string(json, tokens, content, content_str, len);

    Message* message = ArenaAlloc(arena, sizeof(Message));

    message->content = content_str;

    return message;
}
