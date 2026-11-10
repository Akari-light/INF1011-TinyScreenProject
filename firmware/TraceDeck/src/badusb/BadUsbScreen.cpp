#include "BadUsbScreen.h"

#include "../ui/TerminalTheme.h"

namespace {
constexpr uint8_t kArmButton = TSButtonLowerRight;
constexpr uint8_t kCancelOrBackButton = TSButtonLowerLeft;
constexpr uint8_t kInjectButton = TSButtonUpperRight;
constexpr uint32_t kMsPerSecond = 1000;
constexpr uint8_t kArmWindowSeconds = 10;
constexpr uint32_t kArmWindowMs = kArmWindowSeconds * kMsPerSecond;
constexpr uint16_t kSpent = TerminalTheme::color(88, 55, 30);
constexpr uint16_t kCancel = TerminalTheme::color(255, 141, 128);
}

BadUsbScreen::BadUsbScreen(TinyScreen &display) : display_(display) {}

void BadUsbScreen::enter(const BadUsbPayload &payload) {
    payload_ = &payload;
    armed_ = false;
    TerminalTheme::drawFrame(display_);
    drawState(payload_->readyMessage);
}

bool BadUsbScreen::update(uint8_t pressed, uint32_t now) {
    const uint32_t elapsed = now - armedAt_;
    if (armed_ && elapsed >= kArmWindowMs) {
        armed_ = false;
        drawState("Timed out");
    } else if (armed_ && pressed == kCancelOrBackButton) {
        armed_ = false;
        drawState("Cancelled");
    } else if (armed_ && pressed == kInjectButton) {
        armed_ = false;
        if (USBDevice.configured()) {
            clearStatusArea();
            TerminalTheme::drawText(display_, 4, 2, TerminalTheme::kCyan, payload_->label);
            TerminalTheme::drawText(display_, 20, 26, TerminalTheme::kAmber, "Sending keys");
            payload_->run();
            drawState(USBDevice.configured() ? "KEYS SENT" : "USB unavailable");
        } else {
            drawState("USB unavailable");
        }
    } else if (!armed_ && pressed == kArmButton) {
        armed_ = true;
        armedAt_ = now;
        drawState("");
        drawCountdown(kArmWindowSeconds);
    } else if (!armed_ && pressed == kCancelOrBackButton) {
        return true;
    } else if (armed_) {
        // Round up so 01s stays visible until the arming window expires.
        const uint8_t seconds = (kArmWindowMs - elapsed + kMsPerSecond - 1) / kMsPerSecond;
        if (seconds != displayedSeconds_) {
            drawCountdown(seconds);
        }
    }
    return false;
}

void BadUsbScreen::clearStatusArea() {
    // Clear the changing areas while keeping the frame in place.
    display_.clearWindow(4, 2, 91, 10);
    display_.clearWindow(4, 16, 91, 33);
    display_.clearWindow(4, 52, 91, 11);
}

void BadUsbScreen::drawState(const char *message) {
    using namespace TerminalTheme;

    clearStatusArea();

    drawText(display_, 4, 2, kCyan, armed_ ? "BadUSB" : payload_->label);
    drawText(display_, 20, 16, armed_ ? kAmber : kGreen, armed_ ? "ARMED" : "SAFE",
             liberationSansNarrow_14ptFontInfo);

    if (armed_) {
        // Warning triangle with an exclamation point.
        display_.drawLine(11, 18, 18, 31, kAmber);
        display_.drawLine(18, 31, 4, 31, kAmber);
        display_.drawLine(4, 31, 11, 18, kAmber);
        display_.drawLine(11, 22, 11, 26, kAmber);
        display_.drawLine(11, 28, 11, 28, kAmber);

        drawText(display_, 50, 2, kGreen, "INJECT");
        display_.drawLine(88, 3, 92, 7, kGreen);
        display_.drawLine(92, 7, 88, 11, kGreen);
        drawText(display_, 4, 52, kCancel, "CANCEL");
        display_.drawLine(44, 54, 50, 60, kCancel);
        display_.drawLine(50, 54, 44, 60, kCancel);
    } else {
        // Shield and check mark for SAFE.
        display_.drawLine(5, 20, 17, 20, kGreen);
        display_.drawLine(5, 20, 5, 27, kGreen);
        display_.drawLine(17, 20, 17, 27, kGreen);
        display_.drawLine(5, 27, 11, 32, kGreen);
        display_.drawLine(17, 27, 11, 32, kGreen);
        display_.drawLine(8, 25, 10, 27, kGreen);
        display_.drawLine(10, 27, 14, 23, kGreen);

        drawText(display_, 20, 36, kMuted, message);
        drawText(display_, 4, 52, kControl, "BACK");
        drawText(display_, 72, 52, kAmber, "ARM");
    }
}

void BadUsbScreen::drawCountdown(uint8_t seconds) {
    displayedSeconds_ = seconds;
    display_.clearWindow(69, 33, 25, 15);
    char label[] = "00s";
    label[0] = '0' + seconds / 10;
    label[1] = '0' + seconds % 10;
    TerminalTheme::drawText(display_, 70, 33, TerminalTheme::kAmber, label, liberationSans_10ptFontInfo);

    // Each lit tick represents one second remaining.
    for (uint8_t tick = 0; tick < kArmWindowSeconds; ++tick) {
        const uint8_t x = 5 + tick * 6;
        display_.drawRect(x, 40, 4, 2, TSRectangleFilled, tick < seconds ? TerminalTheme::kAmber : kSpent);
    }
}
