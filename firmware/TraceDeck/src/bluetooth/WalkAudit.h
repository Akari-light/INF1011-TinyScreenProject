#pragma once

#include <TinyScreen.h>
#include "BleScanner.h"

class WalkAudit {
public:
    void enter();
    bool update(TinyScreen &display, Ble::Scanner &scanner, uint8_t press);

private:
    void command(char *line, uint32_t now, bool fresh);
    char trial_[33] = {}, input_[64] = {};
    uint8_t used_ = 0;
    bool overflow_ = false, marked_ = false, imported_ = false, haveBeacon_ = false;
    bool confirmEnd_ = false;
    int8_t paintedState_ = -1;
    char paintedRadio_[28] = {};
    uint32_t seenAt_ = 0, markedAt_ = 0, markerAge_ = 0, nextDraw_ = 0;
    int8_t rssi_ = 127, markerRssi_ = 127;
};
