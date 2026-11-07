#pragma once

#include <TinyScreen.h>

class UsbTestScreen {
public:
    explicit UsbTestScreen(TinyScreen &display);

    void enter();
    // Returns true when a Back press requests Home.
    bool update(uint8_t pressed, uint32_t now);

private:
    void drawState(const char *message);
    void drawCountdown(uint8_t seconds);

    TinyScreen &display_;
    bool armed_ = false;
    uint32_t armedAt_ = 0;
    uint8_t displayedSeconds_ = 0;
};
