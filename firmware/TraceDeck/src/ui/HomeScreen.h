#pragma once

#include <TinyScreen.h>

class HomeScreen {
public:
    explicit HomeScreen(TinyScreen &display);

    void enter();
    bool openBadUsbOnPress(uint8_t pressed);

private:
    void drawSelection();

    TinyScreen &display_;
    bool badUsbSelected_ = true;
};
