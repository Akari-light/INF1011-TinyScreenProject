#pragma once

#include "MenuPage.h"

class HomeScreen {
public:
    explicit HomeScreen(TinyScreen &display);

    void enter();
    bool openBadUsbOnPress(uint8_t pressed);

private:
    void drawHint();

    TinyScreen &display_;
    MenuPage menu_;
};
