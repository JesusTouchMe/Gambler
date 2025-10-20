// Copyright 2025 JesusTouchMe

#include "utils/jsonutils.h"

#include <string.h>

bool jsoneq(const char* json, jsmntok_t tok, const char* s) {
    return tok.type == JSMN_STRING && (int) strlen(s) == tok.end - tok.start && strncmp(json + tok.start, s, tok.end - tok.start) == 0;
}

JsonObject jsmn_find_key(const char* json, jsmntok_t* tokens, JsonObject object, const char* key) {
    int i = object + 1;
    int count = tokens[object].size;

    for (int k = 0; k < count; k++) {
        if (jsoneq(json, tokens[i], key)) {
            return i + 1;
        }

        i += jsmn_skip(&tokens[i + 1]) + 2;
    }

    return JSON_NULL;
}

JsonObject jsmn_find_path(const char* json, jsmntok_t* tokens, JsonObject root, const char* path) {
    char key[64];
    int current = root;
    const char* p = path;
    const char* dot = NULL;

    while (*p) {
        dot = strchr(p, '.');
        int len = dot != NULL ? dot - p : strlen(p);
        memcpy(key, p, len);
        key[len] = '\0';

        int val = jsmn_find_key(json, tokens, current, key);
        if (val == JSON_NULL) return JSON_NULL;

        current = val;
        if (dot == NULL) break;
        p = dot + 1;
    }

    return current;
}

int jsmn_skip(const jsmntok_t* token) {
    if (token->type != JSMN_OBJECT && token->type != JSMN_ARRAY) return 0;

    int count = 0;
    for (int i = 0; i < token->size; i++) {
        count += 1 + jsmn_skip(token + 1 + count);
    }

    return count;
}

void jsmn_copy_string(const char* json, jsmntok_t* tokens, JsonObject object, char* dest, size_t dest_size) {
    jsmntok_t* token = &tokens[object];
    int len = token->end - token->start;
    if (len >= dest_size) len = dest_size - 1;
    memcpy(dest, json + token->start, len);
    dest[len] = '\0';
}
