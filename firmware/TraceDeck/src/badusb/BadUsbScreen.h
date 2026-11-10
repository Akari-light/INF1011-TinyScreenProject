#pragma once

#include <TinyScreen.h>

#include "BadUsbPayload.h"

class BadUsbScreen {
public:
    explicit BadUsbScreen(TinyScreen &display);

    void enter(const BadUsbPayload &payload);
    // Returns true when a Back press requests the payload menu.
    bool update(uint8_t pressed, uint32_t now);

private:
    void clearStatusArea();
    void drawState(const char *message);
    void drawCountdown(uint8_t seconds, uint8_t breathFrame);
    void drawGauge(uint8_t seconds, bool redrawOutlines, uint16_t ink);
    void runPayload();
    void drawLoadingProgress(uint8_t percent);

    TinyScreen &display_;
    const BadUsbPayload *payload_ = nullptr;
    bool armed_ = false;
    uint32_t armedAt_ = 0;
    uint8_t displayedSeconds_ = 0;
    uint8_t displayedBreathFrame_ = 0;
};
