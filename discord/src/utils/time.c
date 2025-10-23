// Copyright 2025 JesusTouchMe

#include "utils/time.h"

#include <time.h>

uint64_t NowMs(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t) ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}
