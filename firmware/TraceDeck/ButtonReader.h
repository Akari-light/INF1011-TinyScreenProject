#pragma once

#include <TinyScreen.h>

class ButtonReader {
public:
    explicit ButtonReader(TinyScreen &display);

    void begin(uint32_t now);
    uint8_t readPress(uint32_t now);

private:
    TinyScreen &display_;
    uint32_t buttonsChangedAt_ = 0;
    uint8_t rawButtons_ = 0;
    uint8_t stableButtons_ = 0;
};
