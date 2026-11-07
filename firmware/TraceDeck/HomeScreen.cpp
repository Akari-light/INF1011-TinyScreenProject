#include "HomeScreen.h"

namespace {
constexpr uint8_t kUsbRowY = 18;
constexpr uint8_t kWifiRowY = 33;
}

HomeScreen::HomeScreen(TinyScreen &display) : display_(display) {}

void HomeScreen::enter() {
    display_.clearScreen();
    display_.setFont(thinPixel7_10ptFontInfo);
    display_.fontColor(TS_8b_White, TS_8b_Black);
    display_.setCursor(5, 3);
    display_.print("OPEN");
    display_.setCursor(36, 3);
    display_.print("HOME");
    display_.setCursor(79, 3);
    display_.print("UP");
    display_.setCursor(67, 52);
    display_.print("DOWN");
    display_.setCursor(8, kUsbRowY + 2);
    display_.print("USB TEST");
    display_.setCursor(8, kWifiRowY + 2);
    display_.print("WI-FI SOON");
    drawSelection(TS_8b_White);
}

bool HomeScreen::openUsbOnPress(uint8_t pressed) {
    if (pressed == TSButtonUpperRight || pressed == TSButtonLowerRight) {
        // Either direction advances to the other row in a two-row menu.
        drawSelection(TS_8b_Black);
        usbSelected_ = !usbSelected_;
        drawSelection(TS_8b_White);
    }

    return pressed == TSButtonUpperLeft && usbSelected_;
}

void HomeScreen::drawSelection(uint16_t color) {
    const uint8_t rowY = usbSelected_ ? kUsbRowY : kWifiRowY;
    display_.drawRect(3, rowY, 90, 14, TSRectangleNoFill, color);
}
