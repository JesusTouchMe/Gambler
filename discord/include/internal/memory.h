// Copyright 2025 JesusTouchMe

#ifndef DISCORD_INTERNAL_MEMORY_H
#define DISCORD_INTERNAL_MEMORY_H 1

#include <stddef.h>

typedef struct _Arena* Arena;

Arena ArenaCreate(size_t initial_size);
void ArenaDestroy(Arena arena);
void ArenaReset(Arena* arena);
void* ArenaAlloc(Arena* arena, size_t size);

// The temp arena is reset every time its out of memory. Do not keep pointers around.
Arena* GetTempArena();

// do not use these functions on windows lmao
void* HeapAlloc(size_t size);
void* HeapRealloc(void* ptr, size_t size);
void HeapFree(void* ptr);

char* CopyString(const char* str); // strdup but uses these heap functions

#endif // DISCORD_INTERNAL_MEMORY_H
