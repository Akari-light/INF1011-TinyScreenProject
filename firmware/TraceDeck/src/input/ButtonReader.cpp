#include "ButtonReader.h"

namespace {
constexpr uint32_t kDebounceMs = 25;
}

ButtonReader::ButtonReader(TinyScreen &display) : display_(display) {}

void ButtonReader::begin(uint32_t now) {
    rawButtons_ = stableButtons_ = display_.getButtons();
    buttonsChangedAt_ = now;
}

uint8_t ButtonReader::readPress(uint32_t now) {
    const uint8_t buttons = display_.getButtons();
    if (buttons != rawButtons_) {
        rawButtons_ = buttons;
        buttonsChangedAt_ = now;
    }

    if (rawButtons_ == stableButtons_ || now - buttonsChangedAt_ < kDebounceMs) {
        return 0;
    }

    // Accept a new press only after all previous buttons have been released.
    const uint8_t pressed = stableButtons_ == 0 ? rawButtons_ : 0;
    stableButtons_ = rawButtons_;
    return pressed;
}
