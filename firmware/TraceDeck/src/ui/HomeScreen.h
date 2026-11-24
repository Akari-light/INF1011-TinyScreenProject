#pragma once

#include "MenuPage.h"

class HomeScreen {
public:
    enum class Action { None, BadUsb, Bluetooth };

    explicit HomeScreen(TinyScreen &display);

    void enter();
    Action update(uint8_t pressed);

private:
    TinyScreen &display_;
    MenuPage menu_;
};
