#include "HomeScreen.h"

#include "TerminalTheme.h"

namespace {
constexpr uint8_t kBadUsbRowY = 16;
constexpr uint8_t kWifiRowY = 33;
constexpr uint16_t kIdleBadUsb = TerminalTheme::color(124, 161, 177);
constexpr uint16_t kSelectedWifi = TerminalTheme::color(183, 226, 165);
}

HomeScreen::HomeScreen(TinyScreen &display) : display_(display) {}

void HomeScreen::enter() {
    using namespace TerminalTheme;

    drawFrame(display_);
    display_.drawLine(32, 3, 32, 11, kFrame);
    drawText(display_, 38, 0, kGreen, "HOME", liberationSansNarrow_10ptFontInfo);
    drawText(display_, 64, kWifiRowY + 4, kMuted, "SOON");

    // The arrows sit beside the upper and lower right buttons.
    display_.drawLine(85, 7, 89, 3, kControl);
    display_.drawLine(89, 3, 93, 7, kControl);
    display_.drawLine(89, 3, 89, 11, kControl);
    display_.drawLine(85, 57, 89, 61, kControl);
    display_.drawLine(89, 61, 93, 57, kControl);
    display_.drawLine(89, 53, 89, 61, kControl);
    drawSelection();
}

bool HomeScreen::openBadUsbOnPress(uint8_t pressed) {
    if (pressed == TSButtonUpperRight || pressed == TSButtonLowerRight) {
        // Either direction advances to the other row in a two-row menu.
        badUsbSelected_ = !badUsbSelected_;
        drawSelection();
    }

    return pressed == TSButtonUpperLeft && badUsbSelected_;
}

void HomeScreen::drawSelection() {
    using namespace TerminalTheme;

    drawText(display_, 4, 2, badUsbSelected_ ? kControl : kMuted, "OPEN");
    display_.clearWindow(4, 19, 5, 27);
    const uint8_t pointerY = (badUsbSelected_ ? kBadUsbRowY : kWifiRowY) + 4;
    display_.drawLine(4, pointerY, 8, pointerY + 4, kCyan);
    display_.drawLine(8, pointerY + 4, 4, pointerY + 8, kCyan);

    const uint16_t badUsbInk = badUsbSelected_ ? kCyan : kIdleBadUsb;
    display_.drawLine(20, 18, 20, 29, badUsbInk);
    display_.drawLine(18, 20, 20, 18, badUsbInk);
    display_.drawLine(20, 18, 22, 20, badUsbInk);
    display_.drawLine(16, 23, 16, 25, badUsbInk);
    display_.drawLine(16, 25, 20, 27, badUsbInk);
    display_.drawLine(24, 22, 24, 24, badUsbInk);
    display_.drawLine(20, 26, 24, 24, badUsbInk);
    drawText(display_, 29, kBadUsbRowY, badUsbSelected_ ? kGreen : kIdleBadUsb, "BadUSB",
             liberationSansNarrow_12ptFontInfo);

    const uint16_t wifiInk = badUsbSelected_ ? kMuted : kSelectedWifi;
    display_.drawLine(14, 38, 17, 35, wifiInk);
    display_.drawLine(17, 35, 23, 35, wifiInk);
    display_.drawLine(23, 35, 26, 38, wifiInk);
    display_.drawLine(17, 41, 19, 39, wifiInk);
    display_.drawLine(19, 39, 21, 39, wifiInk);
    display_.drawLine(21, 39, 23, 41, wifiInk);
    display_.drawLine(20, 43, 20, 44, wifiInk);
    drawText(display_, 29, kWifiRowY, wifiInk, "WI-FI", liberationSansNarrow_12ptFontInfo);
}
