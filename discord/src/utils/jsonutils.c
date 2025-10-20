// Copyright 2025 JesusTouchMe

#include "utils/jsonutils.h"

#include <string.h>

bool jsoneq(const char* json, jsmntok_t tok, const char* s) {
    return tok.type == JSMN_STRING && (int) strlen(s) == tok.end - tok.start && strncmp(json + tok.start, s, tok.end - tok.start) == 0;
}

JsonObject jsmn_find_key(const char* json, const jsmntok_t* tokens, JsonObject object, const char* key) {
    JsonObject i = object + 1;
    int count = tokens[object].size;

    for (int k = 0; k < count; k++) {
        if (jsoneq(json, tokens[i], key)) {
            return i + 1;
        }

        JsonObject val = i + 1;
        int skip = jsmn_subtree_size(tokens, val);
        i = val + 1 + skip;
    }

    return JSON_NULL;
}

JsonObject jsmn_find_path(const char* json, const jsmntok_t* tokens, JsonObject root, const char* path) {
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

int jsmn_subtree_size(const jsmntok_t* tokens, JsonObject object) {
    const jsmntok_t* token = &tokens[object];
    int total = 0;

    if (token->type == JSMN_PRIMITIVE || token->type == JSMN_STRING) return 0;

    if (token->type == JSMN_OBJECT) {
        int i = object + 1;
        for (int p = 0; p < token->size; p++) {
            total += 1;
            int val = i + 1;
            int sub = jsmn_subtree_size(tokens, val);
            total += 1 + sub;
            i = val + 1 + sub;
        }
    } else if (token->type == JSMN_ARRAY) {
        int i = object + 1;
        for (int p = 0; p < token->size; p++) {
            int sub = jsmn_subtree_size(tokens, i);
            total += 1 + sub;
            i += 1 + sub;
        }
    }

    return total;
}

void jsmn_copy_string(const char* json, const jsmntok_t* tokens, JsonObject object, char* dest, size_t dest_size) {
    jsmntok_t* token = &tokens[object];
    int len = token->end - token->start;
    if (len >= dest_size) len = dest_size - 1;
    memcpy(dest, json + token->start, len);
    dest[len] = '\0';
}
