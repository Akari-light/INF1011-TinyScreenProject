#pragma once

#include <stdint.h>

struct BadUsbProgress {
    void *context;
    void (*update)(void *context, uint8_t percent);
};

struct BadUsbPayload {
    const char *label;
    void (*run)(const BadUsbProgress &progress);
};
