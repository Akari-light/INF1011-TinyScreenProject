#include "HomeScreen.h"

#include "TerminalTheme.h"

namespace {
constexpr size_t kBadUsbEntry = 0;
constexpr const char *kLabels[] = {
    "BADUSB",
    "BLUETOOTH",
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
    TerminalTheme::drawControl(display_, 4, 3, TerminalTheme::kControl, "OPEN");
}

HomeScreen::Action HomeScreen::update(uint8_t pressed) {
    menu_.move(pressed);
    if (pressed != TSButtonUpperLeft) return Action::None;
    return menu_.selected() == kBadUsbEntry ? Action::BadUsb : Action::Bluetooth;
}
