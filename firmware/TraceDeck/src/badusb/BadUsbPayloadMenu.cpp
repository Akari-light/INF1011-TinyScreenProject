#include "BadUsbPayloadMenu.h"

#include "../ui/TerminalTheme.h"
#include "payloads/drop_upload/DropUpload.h"
#include "payloads/de_shield/DeShield.h"
#include "payloads/pwning/Pwning.h"
#include "payloads/payload/Payload.h"
#include "payloads/edge_jar/CookieStealer.h"
#include "payloads/chrome_jar/ChromeCookieStealer.h"

namespace {
const BadUsbPayload kPayloads[] = {
    {"Drop & Upload", sendDropUploadKeys},
    {"De-Shield", sendDeShieldKeys},
    {"Pwning", sendPwningKeys},
    {"Edge-Jar", sendCookieStealerKeys},
    {"Chrm-Jar", sendChromeCookieStealerKeys},
    {"Payload", sendPayloadKeys},
};

const char *payloadLabelAt(size_t index) {
    return kPayloads[index].label;
}
}

BadUsbPayloadMenu::BadUsbPayloadMenu(TinyScreen &display)
    : display_(display), menu_(display, payloadLabelAt, sizeof(kPayloads) / sizeof(kPayloads[0])) {}

void BadUsbPayloadMenu::enter() {
    using namespace TerminalTheme;
    menu_.enter();
    drawText(display_, 34, 2, kGreen, "PAYLOADS");
    drawControl(display_, 4, 54, kControl, "BACK");
    drawOpenHint();
}

BadUsbPayloadMenu::Action BadUsbPayloadMenu::update(uint8_t pressed) {
    if (pressed == TSButtonUpperLeft) {
        return selectedPayload().run ? Action::Open : Action::None;
    }
    if (pressed == TSButtonLowerLeft) {
        return Action::Back;
    }
    if (menu_.move(pressed)) {
        drawOpenHint();
    }
    return Action::None;
}

const BadUsbPayload &BadUsbPayloadMenu::selectedPayload() const {
    return kPayloads[menu_.selected()];
}

void BadUsbPayloadMenu::drawOpenHint() {
    using namespace TerminalTheme;
    drawControl(display_, 4, 3, selectedPayload().run ? kControl : kMuted, "OPEN");
}
