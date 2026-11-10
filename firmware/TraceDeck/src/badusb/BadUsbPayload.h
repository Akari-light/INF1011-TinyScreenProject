#pragma once

struct BadUsbPayload {
    const char *label;
    const char *readyMessage;
    void (*run)();
};
