#include "Payload3.h"

#include <Keyboard.h>

namespace {
constexpr char kTestMessage[] = "TraceDeck payload 3 test";
constexpr uint32_t kTypingWaitMs = 1000;
constexpr uint32_t kProgressRefreshMs = 125;

// Split a pause into short waits so the OLED can redraw its progress bar.
void waitWithProgress(const BadUsbProgress &progress, uint32_t durationMs) {
    const uint32_t startedAt = millis();
    uint32_t elapsedMs = 0;
    while (elapsedMs < durationMs) {
        const uint32_t remainingMs = durationMs - elapsedMs;
        delay(remainingMs < kProgressRefreshMs ? remainingMs : kProgressRefreshMs);
        elapsedMs = millis() - startedAt;
        const uint32_t waitedMs = elapsedMs < durationMs ? elapsedMs : durationMs;
        // The screen draws 100% after the payload returns.
        progress.update(progress.context, waitedMs * 99 / durationMs);
        elapsedMs = millis() - startedAt;
    }
}
}

void sendPayload3Keys(const BadUsbProgress &progress) {
    if (!USBDevice.configured()) {
        return;
    }

    Keyboard.begin(KeyboardLayout_en_US);
    progress.update(progress.context, 0);

    // Replace this fixed marker with the steps for the next approved demo.
    Keyboard.print(kTestMessage);
    Keyboard.releaseAll();
    waitWithProgress(progress, kTypingWaitMs);
}
