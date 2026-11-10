#pragma once

#include <TinyScreen.h>

#include "BadUsbPayload.h"

class BadUsbPayloadMenu {
public:
    enum class Action { None, Open, Back };

    explicit BadUsbPayloadMenu(TinyScreen &display);

    void enter();
    Action update(uint8_t pressed);
    const BadUsbPayload &selectedPayload() const;

private:
    void drawSelection();

    TinyScreen &display_;
    uint8_t selected_ = 0;
};
