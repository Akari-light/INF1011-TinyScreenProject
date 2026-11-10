#include "BadUsbPayloadMenu.h"

#include "../ui/TerminalTheme.h"
#include "payloads/drop_upload/DropUpload.h"

namespace {
const BadUsbPayload kPayloads[] = {
    {"Drop & Upload", dropUploadHasTraceHubUrl() ? "Upload enabled" : "Notepad only",
     sendBadUsbDemoKeys},
    {"Payload 2", nullptr, nullptr},
    {"Payload 3", nullptr, nullptr},
};
constexpr uint8_t kPayloadCount = sizeof(kPayloads) / sizeof(kPayloads[0]);
constexpr uint8_t kFirstRowY = 16;
constexpr uint8_t kRowStep = 12;
}

BadUsbPayloadMenu::BadUsbPayloadMenu(TinyScreen &display) : display_(display) {}

void BadUsbPayloadMenu::enter() {
    using namespace TerminalTheme;

    drawFrame(display_);
    display_.drawLine(32, 3, 32, 11, kFrame);
    drawText(display_, 35, 2, kGreen, "PAYLOADS");
    drawText(display_, 4, 52, kControl, "BACK");

    // Match Home's arrows beside the upper and lower right buttons.
    display_.drawLine(85, 7, 89, 3, kControl);
    display_.drawLine(89, 3, 93, 7, kControl);
    display_.drawLine(89, 3, 89, 11, kControl);
    display_.drawLine(85, 57, 89, 61, kControl);
    display_.drawLine(89, 61, 93, 57, kControl);
    display_.drawLine(89, 53, 89, 61, kControl);
    drawSelection();
}

BadUsbPayloadMenu::Action BadUsbPayloadMenu::update(uint8_t pressed) {
    switch (pressed) {
    case TSButtonUpperLeft:
        return selectedPayload().run ? Action::Open : Action::None;
    case TSButtonLowerLeft:
        return Action::Back;
    case TSButtonUpperRight:
        selected_ = (selected_ + kPayloadCount - 1) % kPayloadCount;
        break;
    case TSButtonLowerRight:
        selected_ = (selected_ + 1) % kPayloadCount;
        break;
    default:
        return Action::None;
    }

    drawSelection();
    return Action::None;
}

const BadUsbPayload &BadUsbPayloadMenu::selectedPayload() const {
    return kPayloads[selected_];
}

void BadUsbPayloadMenu::drawSelection() {
    using namespace TerminalTheme;

    drawText(display_, 4, 2, selectedPayload().run ? kControl : kMuted, "OPEN");
    display_.clearWindow(4, kFirstRowY, 5, 34);
    const uint8_t pointerY = kFirstRowY + selected_ * kRowStep + 1;
    display_.drawLine(4, pointerY, 8, pointerY + 4, kCyan);
    display_.drawLine(8, pointerY + 4, 4, pointerY + 8, kCyan);

    for (uint8_t row = 0; row < kPayloadCount; ++row) {
        const auto &payload = kPayloads[row];
        const uint8_t y = kFirstRowY + row * kRowStep;
        drawText(display_, 14, y, row == selected_ ? kGreen : kMuted, payload.label);
        if (!payload.run) {
            drawText(display_, 68, y, kMuted, "SOON");
        }
    }
}
