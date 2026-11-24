#pragma once

#include <TinyScreen.h>

#include "BleScanner.h"
#include "WalkAudit.h"

class BleNearbyScreen {
public:
    explicit BleNearbyScreen(TinyScreen &display) : display_(display) {}
    void enter(bool audit = false);
    bool update(uint8_t pressed);

private:
    struct Line {
        char text[32];
        char signal[7];
        uint16_t ink;
        bool selected;
    };

    void list(uint32_t now, const uint16_t *ordered, uint8_t count, uint8_t index, Ble::RecentAddresses recent);
    void detail(uint32_t now, const Ble::Observation *entry, Ble::RecentAddresses recent);
    void arrows();
    void draw(const char *title, const char *action, const char *footer, bool scrollable = false);
    void log(uint32_t now, Ble::RecentAddresses recent);

    TinyScreen &display_;
    Ble::Scanner scanner_;
    WalkAudit audit_;
    bool auditing_ = false;
    uint32_t lastDraw_ = 0, lastLog_ = 0;
    uint16_t selected_ = 0;
    bool details_ = false, explicitSelection_ = false;
    bool forceDraw_ = true;
    bool previousScrollable_ = false;
    Line lines_[3] = {}, previous_[3] = {};
    char previousTitle_[20] = {}, previousAction_[8] = {}, previousFooter_[24] = {};
};
