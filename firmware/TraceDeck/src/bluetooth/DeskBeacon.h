#pragma once

#include <TinyScreen.h>

class DeskBeacon {
public:
    explicit DeskBeacon(TinyScreen &display) : display_(display) {}
    void enter();
    bool update(uint8_t pressed);

private:
    int start();
    void setAdvertising(bool enabled);
    void draw();

    TinyScreen &display_;
    const char *stage_ = "";
    int status_ = 0;
    uint8_t hardware_ = 0;
    uint16_t firmware_ = 0;
    uint32_t lastPrint_ = 0;
    bool ready_ = false, advertising_ = false, ignoreSerialLine_ = false;
};
