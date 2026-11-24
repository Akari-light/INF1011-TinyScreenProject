#pragma once

#include "../ui/MenuPage.h"
#include "BleNearbyScreen.h"
#include "DeskBeacon.h"

class BluetoothMenu {
public:
    explicit BluetoothMenu(TinyScreen &display);
    void enter();
    bool update(uint8_t pressed);

private:
    enum class Page { Menu, WalkAway, Beacon, Nearby };
    struct Entry { const char *label; Page page; };
    static const Entry entries_[];
    static const char *labelAt(size_t index);
    TinyScreen &display_;
    MenuPage menu_;
    BleNearbyScreen receiver_;
    DeskBeacon beacon_;
    Page page_ = Page::Menu;
};
