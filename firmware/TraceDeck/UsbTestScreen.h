#pragma once

#include <TinyScreen.h>

class UsbTestScreen {
public:
    explicit UsbTestScreen(TinyScreen &display);

    void enter();
    void update();

private:
    void draw(const char *message);

    TinyScreen &display_;
    bool armed_ = false;
    uint32_t armedAt_ = 0;
    uint32_t buttonsChangedAt_ = 0;
    uint8_t rawButtons_ = 0;
    uint8_t stableButtons_ = 0;
};
