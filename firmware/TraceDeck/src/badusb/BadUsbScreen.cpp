#include "BadUsbScreen.h"

#include "../ui/TerminalTheme.h"

namespace {
constexpr uint8_t kArmButton = TSButtonLowerRight;
constexpr uint8_t kCancelOrBackButton = TSButtonLowerLeft;
constexpr uint8_t kInjectButton = TSButtonUpperRight;
constexpr uint32_t kMsPerSecond = 1000;
constexpr uint8_t kSegmentCount = 10;
constexpr uint8_t kSecondsPerSegment = 6;
constexpr uint8_t kArmWindowSeconds = kSegmentCount * kSecondsPerSegment;
constexpr uint32_t kArmWindowMs = kArmWindowSeconds * kMsPerSecond;
constexpr uint8_t kGaugeX = 4;
constexpr uint8_t kGaugeTop = 35;
constexpr uint8_t kRowsPerSecond = 2;
constexpr uint8_t kGaugeHeight = kSecondsPerSegment * kRowsPerSecond;
constexpr uint8_t kGaugeBottom = kGaugeTop + kGaugeHeight - 1;
constexpr uint8_t kSegmentStep = 6;
constexpr uint8_t kSegmentWidth = 5;
constexpr uint8_t kSegmentSlant = 2;
constexpr uint8_t kGaugeWidth = (kSegmentCount - 1) * kSegmentStep + kSegmentWidth + kSegmentSlant;
constexpr uint16_t kSpentOutline = TerminalTheme::color(105, 68, 34);
constexpr uint8_t kBreathHalfFrames = 10;
constexpr uint32_t kBreathFrameMs = 100;
constexpr uint8_t kUrgentGreen = 88;
constexpr uint8_t kUrgentBlue = 47;
constexpr uint16_t kCancel = TerminalTheme::color(255, 141, 128);
constexpr uint8_t kLoadingBarX = 4;
constexpr uint8_t kLoadingBarY = 37;
constexpr uint8_t kLoadingBarWidth = 89;
constexpr uint8_t kLoadingBarHeight = 10;
constexpr uint8_t kSpinnerDots[][2] = {{86, 19}, {91, 24}, {86, 29}, {81, 24}};
constexpr uint8_t kSpinnerDotCount = sizeof(kSpinnerDots) / sizeof(kSpinnerDots[0]);
constexpr uint32_t kSpinnerFrameMs = 125;

uint16_t countdownInk(uint8_t seconds, uint8_t breathFrame) {
    if (seconds <= kSecondsPerSegment) {
        const uint8_t dimStep = breathFrame <= kBreathHalfFrames
                                    ? breathFrame
                                    : 2 * kBreathHalfFrames - breathFrame;
        const uint8_t brightness = 100 - 3 * dimStep;
        return TerminalTheme::color(255 * brightness / 100, kUrgentGreen * brightness / 100,
                                    kUrgentBlue * brightness / 100);
    }

    const uint8_t fadeSeconds = kArmWindowSeconds - kSecondsPerSegment;
    const uint8_t secondsAboveUrgent = seconds - kSecondsPerSegment;
    const uint8_t green = kUrgentGreen + (187 - kUrgentGreen) * secondsAboveUrgent / fadeSeconds;
    const uint8_t blue = kUrgentBlue + (70 - kUrgentBlue) * secondsAboveUrgent / fadeSeconds;
    return TerminalTheme::color(255, green, blue);
}
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
            runPayload();
        } else {
            drawState("USB unavailable");
        }
    } else if (!armed_ && pressed == kArmButton) {
        armed_ = true;
        armedAt_ = now;
        drawState("");
        displayedSeconds_ = 0;
        drawCountdown(kArmWindowSeconds, 0);
    } else if (!armed_ && pressed == kCancelOrBackButton) {
        return true;
    } else if (armed_) {
        // Round up so 01s stays visible until the arming window expires.
        const uint8_t seconds = (kArmWindowMs - elapsed + kMsPerSecond - 1) / kMsPerSecond;
        const uint8_t breathFrame = seconds <= kSecondsPerSegment
            ? (elapsed / kBreathFrameMs) % (2 * kBreathHalfFrames)
            : 0;
        if (seconds != displayedSeconds_ || breathFrame != displayedBreathFrame_) {
            drawCountdown(seconds, breathFrame);
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

void BadUsbScreen::drawCountdown(uint8_t seconds, uint8_t breathFrame) {
    const bool secondsChanged = seconds != displayedSeconds_;
    const uint16_t ink = countdownInk(seconds, breathFrame);
    drawGauge(seconds, secondsChanged, ink);
    if (secondsChanged) {
        display_.clearWindow(69, 34, 26, 14);
    }
    displayedSeconds_ = seconds;
    displayedBreathFrame_ = breathFrame;

    char label[] = "00s";
    label[0] = '0' + seconds / 10;
    label[1] = '0' + seconds % 10;
    TerminalTheme::drawText(display_, 70, 34, ink, label, liberationSans_10ptFontInfo);
}

void BadUsbScreen::drawGauge(uint8_t seconds, bool redrawOutlines, uint16_t ink) {
    if (redrawOutlines) {
        display_.clearWindow(kGaugeX, kGaugeTop, kGaugeWidth, kGaugeHeight);
    }

    const uint8_t fullSegments = seconds / kSecondsPerSegment;
    const uint8_t partialRows = (seconds % kSecondsPerSegment) * kRowsPerSecond;
    const uint8_t drawCount = redrawOutlines ? kSegmentCount : fullSegments + (partialRows > 0);
    for (uint8_t segment = 0; segment < drawCount; ++segment) {
        const uint8_t left = kGaugeX + segment * kSegmentStep;
        const uint8_t topLeft = left + kSegmentSlant;
        if (segment < fullSegments) {
            for (uint8_t column = 0; column < kSegmentWidth; ++column) {
                display_.drawLine(topLeft + column, kGaugeTop, left + column, kGaugeBottom, ink);
            }
            continue;
        }

        if (redrawOutlines) {
            const uint8_t right = left + kSegmentWidth - 1;
            const uint8_t topRight = right + kSegmentSlant;
            display_.drawLine(topLeft, kGaugeTop, topRight, kGaugeTop, kSpentOutline);
            display_.drawLine(left, kGaugeBottom, right, kGaugeBottom, kSpentOutline);
            display_.drawLine(topLeft, kGaugeTop, left, kGaugeBottom, kSpentOutline);
            display_.drawLine(topRight, kGaugeTop, right, kGaugeBottom, kSpentOutline);
        }

        if (segment == fullSegments) {
            // Fill the remaining two rows per second from the bottom of the slanted box.
            for (uint8_t row = 0; row < partialRows; ++row) {
                const uint8_t y = kGaugeBottom - row;
                const uint8_t offset = (row * kSegmentSlant + (kGaugeHeight - 1) / 2) / (kGaugeHeight - 1);
                const uint8_t rowLeft = left + offset;
                display_.drawLine(rowLeft, y, rowLeft + kSegmentWidth - 1, y, ink);
            }
        }
    }
}

void BadUsbScreen::runPayload() {
    using namespace TerminalTheme;

    clearStatusArea();
    drawText(display_, 4, 2, kCyan, payload_->label);
    drawText(display_, 4, 16, kAmber, "INJECTING", liberationSansNarrow_14ptFontInfo);
    display_.drawRect(kLoadingBarX, kLoadingBarY, kLoadingBarWidth, kLoadingBarHeight,
                      TSRectangleNoFill, kFrame);
    drawLoadingProgress(0);

    payload_->run({this, [](void *context, uint8_t percent) {
        static_cast<BadUsbScreen *>(context)->drawLoadingProgress(percent);
    }});

    const bool usbConfigured = USBDevice.configured();
    if (usbConfigured) {
        drawLoadingProgress(100);
    }
    drawState(usbConfigured ? "KEYS SENT" : "USB unavailable");
}

void BadUsbScreen::drawLoadingProgress(uint8_t percent) {
    const uint8_t active = (millis() / kSpinnerFrameMs) % kSpinnerDotCount;
    for (uint8_t dot = 0; dot < kSpinnerDotCount; ++dot) {
        display_.drawRect(kSpinnerDots[dot][0], kSpinnerDots[dot][1], 2, 2, TSRectangleFilled,
                          dot == active ? TerminalTheme::kAmber : kSpentOutline);
    }

    const uint8_t width = (kLoadingBarWidth - 2) * percent / 100;
    if (width > 0) {
        display_.drawRect(kLoadingBarX + 1, kLoadingBarY + 1, width, kLoadingBarHeight - 2,
                          TSRectangleFilled, TerminalTheme::kAmber);
    }
}
