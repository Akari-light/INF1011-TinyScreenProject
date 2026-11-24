#include "BluetoothMenu.h"
#include "../ui/TerminalTheme.h"

const BluetoothMenu::Entry BluetoothMenu::entries_[] = {
    {"Walk-away", Page::WalkAway},
    {"Desk beacon", Page::Beacon},
    {"Nearby scan", Page::Nearby},
};

const char *BluetoothMenu::labelAt(size_t index) {
    return entries_[index].label;
}

BluetoothMenu::BluetoothMenu(TinyScreen &display)
    : display_(display), menu_(display, labelAt, sizeof(entries_) / sizeof(entries_[0])),
      receiver_(display), beacon_(display) {}

void BluetoothMenu::enter() {
    using namespace TerminalTheme;
    page_ = Page::Menu;
    menu_.enter();
    drawText(display_, 30, 2, kCyan, "BLUETOOTH");
    drawControl(display_, 4, 3, kControl, "OPEN");
    drawControl(display_, 4, 54, kControl, "BACK");
}

bool BluetoothMenu::update(uint8_t pressed) {
    switch (page_) {
    case Page::Menu:
        if (pressed == TSButtonLowerLeft) return true;
        menu_.move(pressed);
        if (pressed == TSButtonUpperLeft) {
            page_ = entries_[menu_.selected()].page;
            if (page_ == Page::Beacon) beacon_.enter();
            else receiver_.enter(page_ == Page::WalkAway);
        }
        break;
    case Page::Beacon:
        if (beacon_.update(pressed)) enter();
        break;
    case Page::WalkAway:
    case Page::Nearby:
        if (receiver_.update(pressed)) enter();
        break;
    }
    return false;
}
