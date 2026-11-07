#include "UsbTestScreen.h"

#include <Keyboard.h>

#include "TerminalTheme.h"

namespace {
constexpr uint8_t kArmButton = TSButtonLowerLeft;
constexpr uint8_t kCancelOrBackButton = TSButtonLowerRight;
constexpr uint8_t kSendButton = TSButtonUpperRight;
constexpr uint32_t kMsPerSecond = 1000;
constexpr uint8_t kArmWindowSeconds = 10;
constexpr uint32_t kArmWindowMs = kArmWindowSeconds * kMsPerSecond;
constexpr char kTestMarker[] = "tracedeck usb test";
constexpr uint16_t kSpent = TerminalTheme::color(88, 55, 30);
constexpr uint16_t kCancel = TerminalTheme::color(255, 141, 128);
}

UsbTestScreen::UsbTestScreen(TinyScreen &display) : display_(display) {}

void UsbTestScreen::enter() {
    armed_ = false;
    Keyboard.begin(KeyboardLayout_en_US);
    TerminalTheme::drawFrame(display_);
    TerminalTheme::drawText(display_, 4, 2, TerminalTheme::kCyan, "USB TEST");
    drawState("Ready");
}

bool UsbTestScreen::update(uint8_t pressed, uint32_t now) {
    const uint32_t elapsed = now - armedAt_;
    if (armed_ && elapsed >= kArmWindowMs) {
        armed_ = false;
        drawState("Timed out");
    } else if (armed_ && pressed == kCancelOrBackButton) {
        armed_ = false;
        drawState("Cancelled");
    } else if (armed_ && pressed == kSendButton) {
        armed_ = false;
        if (USBDevice.configured()) {
            Keyboard.print(kTestMarker);
            drawState("Send attempted");
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

void UsbTestScreen::drawState(const char *message) {
    using namespace TerminalTheme;

    // Clear the changing areas while keeping the frame and title in place.
    display_.clearWindow(61, 2, 33, 10);
    display_.clearWindow(4, 16, 91, 33);
    display_.clearWindow(4, 52, 91, 11);

    drawText(display_, 20, 16, armed_ ? kAmber : kGreen, armed_ ? "ARMED" : "SAFE",
             liberationSansNarrow_14ptFontInfo);

    if (armed_) {
        // Warning triangle with an exclamation point.
        display_.drawLine(11, 18, 18, 31, kAmber);
        display_.drawLine(18, 31, 4, 31, kAmber);
        display_.drawLine(4, 31, 11, 18, kAmber);
        display_.drawLine(11, 22, 11, 26, kAmber);
        display_.drawLine(11, 28, 11, 28, kAmber);

        drawText(display_, 61, 2, kGreen, "SEND");
        display_.drawLine(88, 3, 92, 7, kGreen);
        display_.drawLine(92, 7, 88, 11, kGreen);
        drawText(display_, 48, 52, kCancel, "CANCEL");
        display_.drawLine(87, 54, 93, 60, kCancel);
        display_.drawLine(93, 54, 87, 60, kCancel);
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
        drawText(display_, 4, 52, kAmber, "ARM");
        drawText(display_, 61, 52, kControl, "BACK");
        display_.drawLine(91, 53, 87, 57, kControl);
        display_.drawLine(87, 57, 91, 61, kControl);
    }
}

void UsbTestScreen::drawCountdown(uint8_t seconds) {
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
