// Copyright 2025 JesusTouchMe

#include "internal/memory.h"

#include <stdlib.h>
#include <string.h>

#define ARENA_DEFAULT_CHUNK_SIZE 1048576 // 1mb

struct _Arena {
    size_t size;
    size_t used;
    Arena next;
    char data[];
};

static Arena g_temp_arena = NULL;

Arena ArenaCreate(size_t initial_size) {
    if (initial_size == 0) initial_size = ARENA_DEFAULT_CHUNK_SIZE;

    Arena arena = HeapAlloc(sizeof(arena) + initial_size);
    arena->size = initial_size;
    arena->used = 0;
    arena->next = NULL;
    return arena;
}

void ArenaDestroy(Arena arena) {
    while (arena != NULL) {
        Arena temp = arena;
        arena = arena->next;
        HeapFree(temp);
    }
}

void ArenaReset(Arena* arena_p) {
    if (arena_p == NULL) return;

    Arena arena = *arena_p;
    if (arena == NULL) return;

    while (arena->next != NULL) {
        Arena temp = arena;
        arena = arena->next;
        HeapFree(temp);
    }

    arena->used = 0;
    arena->next = NULL;
    *arena_p = arena;
}

void* ArenaAlloc(Arena* arena_p, size_t size) {
    size = (size + 7) & ~7;
    Arena arena = *arena_p;

    if (arena->used + size > arena->size) {
        if (arena == g_temp_arena) {
            if (arena->size < size) {
                ArenaDestroy(g_temp_arena);
                g_temp_arena = ArenaCreate(size + ARENA_DEFAULT_CHUNK_SIZE);
                arena = g_temp_arena;
            } else {
                arena->used = 0;
            }
        } else {
            size_t new_chunk_size = ARENA_DEFAULT_CHUNK_SIZE;
            if (new_chunk_size < size) new_chunk_size += size;

            Arena new_arena = ArenaCreate(new_chunk_size);
            new_arena->next = arena;

            *arena_p = new_arena;
            arena = new_arena;
        }
    }

    void* ptr = arena->data + arena->used;
    arena->used += size;
    return ptr;
}

Arena* GetTempArena() {
    if (g_temp_arena == NULL) g_temp_arena = ArenaCreate(10485760); // it's 10mb. might change latuh
    return &g_temp_arena;
}

void* HeapAlloc(size_t size) {
    void* ptr = malloc(size);
    if (ptr == NULL) {
        exit(42);
    }
    memset(ptr, 0, size);
    return ptr;
}

void* HeapRealloc(void* ptr, size_t size) {
    void* new_ptr = realloc(ptr, size);
    if (new_ptr == NULL) {
        exit(43);
    }
    return new_ptr;
}

void HeapFree(void* ptr) {
    if (ptr != NULL) free(ptr);
}

char* CopyString(const char* str) {
    size_t len = strlen(str);
    char* result = HeapAlloc(len + 1);
    memcpy(result, str, len);
    result[len] = 0;
    return result;
}
