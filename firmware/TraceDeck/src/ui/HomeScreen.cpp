#include "HomeScreen.h"

#include "TerminalTheme.h"

namespace {
constexpr size_t kBadUsbEntry = 0;
constexpr const char *kLabels[] = {
    "BADUSB",
    "WI-FI SOON",
};

const char *homeLabelAt(size_t index) {
    return kLabels[index];
}
}

HomeScreen::HomeScreen(TinyScreen &display)
    : display_(display), menu_(display, homeLabelAt, sizeof(kLabels) / sizeof(kLabels[0])) {}

void HomeScreen::enter() {
    menu_.enter();
    TerminalTheme::drawText(display_, 30, 2, TerminalTheme::kGreen, "TRACEDECK");
    drawHint();
}

bool HomeScreen::openBadUsbOnPress(uint8_t pressed) {
    if (menu_.move(pressed)) {
        drawHint();
    }
    return pressed == TSButtonUpperLeft && menu_.selected() == kBadUsbEntry;
}

void HomeScreen::drawHint() {
    using namespace TerminalTheme;
    const bool badUsbSelected = menu_.selected() == kBadUsbEntry;
    drawControl(display_, 4, 3, badUsbSelected ? kControl : kMuted, "OPEN");
    display_.clearWindow(4, 54, 44, 7);
    if (!badUsbSelected) {
        drawControl(display_, 4, 54, kMuted, "INACTIVE");
    }
}
