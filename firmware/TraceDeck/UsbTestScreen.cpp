#include "UsbTestScreen.h"

namespace {
constexpr uint8_t kArmButton = TSButtonLowerLeft;
constexpr uint8_t kCancelButton = TSButtonLowerRight;
constexpr uint32_t kDebounceMs = 25;
constexpr uint32_t kArmWindowMs = 10000;
}

UsbTestScreen::UsbTestScreen(TinyScreen &display) : display_(display) {}

void UsbTestScreen::enter() {
    armed_ = false;
    rawButtons_ = stableButtons_ = display_.getButtons();
    buttonsChangedAt_ = millis();
    draw("Ready");
}

void UsbTestScreen::update() {
    const uint32_t now = millis();
    const uint8_t buttons = display_.getButtons();
    if (buttons != rawButtons_) {
        rawButtons_ = buttons;
        buttonsChangedAt_ = now;
    }

    uint8_t pressed = 0;
    if (rawButtons_ != stableButtons_ && now - buttonsChangedAt_ >= kDebounceMs) {
        // Accept a new press only after the previous buttons have been released.
        if (stableButtons_ == 0) {
            pressed = rawButtons_;
        }
        stableButtons_ = rawButtons_;
    }

    if (armed_ && now - armedAt_ >= kArmWindowMs) {
        armed_ = false;
        draw("Timed out");
    } else if (armed_ && pressed == kCancelButton) {
        armed_ = false;
        draw("Cancelled");
    } else if (!armed_ && pressed == kArmButton) {
        armed_ = true;
        armedAt_ = now;
        draw("Cancel or wait");
    }
}

void UsbTestScreen::draw(const char *message) {
    display_.clearScreen();
    display_.setFont(thinPixel7_10ptFontInfo);
    display_.fontColor(TS_8b_White, TS_8b_Black);
    display_.setCursor(3, 3);
    display_.print("USB Test");

    display_.fontColor(armed_ ? TS_8b_Yellow : TS_8b_Green, TS_8b_Black);
    display_.setCursor(3, 18);
    display_.print(armed_ ? "ARMED" : "SAFE");

    display_.fontColor(TS_8b_White, TS_8b_Black);
    display_.setCursor(3, 33);
    display_.print(message);
    display_.setCursor(armed_ ? 57 : 3, 52);
    display_.print(armed_ ? "CANCEL" : "ARM");
}
