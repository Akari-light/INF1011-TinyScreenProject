#pragma once

#include <TinyScreen.h>

class HomeScreen {
public:
    explicit HomeScreen(TinyScreen &display);

    void enter();
    bool openUsbOnPress(uint8_t pressed);

private:
    void drawSelection();

    TinyScreen &display_;
    bool usbSelected_ = true;
};
