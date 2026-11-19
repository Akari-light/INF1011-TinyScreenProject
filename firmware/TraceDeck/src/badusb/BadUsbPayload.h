#pragma once

#include <stdint.h>

struct BadUsbProgress {
    void *context;
    void (*update)(void *context, uint8_t percent);
    void (*showDisabled)(void *context);

    BadUsbProgress(void *screenContext, void (*updateProgress)(void *, uint8_t),
                   void (*displayDisabled)(void *) = nullptr)
        : context(screenContext), update(updateProgress), showDisabled(displayDisabled) {}
};

struct BadUsbPayload {
    const char *label;
    void (*run)(const BadUsbProgress &progress);
};
