#pragma once

#include "../ui/MenuPage.h"
#include "BadUsbPayload.h"

class BadUsbPayloadMenu {
public:
    enum class Action { None, Open, Back };

    explicit BadUsbPayloadMenu(TinyScreen &display);

    void enter();
    Action update(uint8_t pressed);
    const BadUsbPayload &selectedPayload() const;

private:
    void drawOpenHint();

    TinyScreen &display_;
    MenuPage menu_;
};
