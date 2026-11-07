#include "UsbTestScreen.h"

#include <Keyboard.h>

namespace {
constexpr uint8_t kArmButton = TSButtonLowerLeft;
constexpr uint8_t kCancelButton = TSButtonLowerRight;
constexpr uint8_t kSendButton = TSButtonUpperRight;
constexpr uint32_t kArmWindowMs = 10000;
constexpr char kTestMarker[] = "tracedeck usb test";
}

UsbTestScreen::UsbTestScreen(TinyScreen &display) : display_(display) {}

void UsbTestScreen::enter() {
    armed_ = false;
    Keyboard.begin(KeyboardLayout_en_US);
    draw("Ready");
}

void UsbTestScreen::update(uint8_t pressed, uint32_t now) {
    if (armed_ && now - armedAt_ >= kArmWindowMs) {
        armed_ = false;
        draw("Timed out");
    } else if (armed_ && pressed == kCancelButton) {
        armed_ = false;
        draw("Cancelled");
    } else if (armed_ && pressed == kSendButton) {
        armed_ = false;
        if (USBDevice.configured()) {
            Keyboard.print(kTestMarker);
            draw("Send attempted");
        } else {
            draw("USB unavailable");
        }
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
    if (armed_) {
        display_.setCursor(69, 3);
        display_.print("SEND");
    }

    display_.fontColor(armed_ ? TS_8b_Yellow : TS_8b_Green, TS_8b_Black);
    display_.setCursor(3, 18);
    display_.print(armed_ ? "ARMED" : "SAFE");

    display_.fontColor(TS_8b_White, TS_8b_Black);
    display_.setCursor(3, 33);
    display_.print(message);
    display_.setCursor(armed_ ? 57 : 3, 52);
    display_.print(armed_ ? "CANCEL" : "ARM");
}
