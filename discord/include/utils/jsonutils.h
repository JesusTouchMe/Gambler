// Copyright 2025 JesusTouchMe

#ifndef DISCORD_UTILS_JSONUTILS_H
#define DISCORD_UTILS_JSONUTILS_H 1

#define JSMN_HEADER
#include "jsmn.h"

#include <stdbool.h>

#define JSON_NULL -1

typedef int JsonObject;

bool jsoneq(const char* json, jsmntok_t tok, const char* s);

// jsmn extensions

// Given the index of an object, find a property inside it and return the index of the value. tokens[obj_index].type MUST be JSMN_OBJECT
JsonObject jsmn_find_key(const char* json, const  jsmntok_t* tokens, JsonObject object, const char* key);

// More structured version of the above if you know the full path. tokens[obj_index].type MUST be JSMN_OBJECT
JsonObject jsmn_find_path(const char* json, const jsmntok_t* tokens, JsonObject root, const char* path);

// Lets you skip through nested arrays/objects
int jsmn_subtree_size(const jsmntok_t* tokens, JsonObject object);

void jsmn_copy_string(const char* json, const jsmntok_t* tokens, JsonObject object, char* dest, size_t dest_size);

#endif // DISCORD_UTILS_JSONUTILS_H
