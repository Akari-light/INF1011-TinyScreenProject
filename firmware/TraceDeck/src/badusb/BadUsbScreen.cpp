#include "BadUsbScreen.h"

#include "../ui/TerminalTheme.h"

namespace {
constexpr uint8_t kArmButton = TSButtonLowerRight;
constexpr uint8_t kCancelOrBackButton = TSButtonLowerLeft;
constexpr uint8_t kModeOrInjectButton = TSButtonUpperRight;
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
    mode_ = Mode::Passive;
    armed_ = false;
    usbWatch_.reset(millis());
    TerminalTheme::drawFrame(display_);
    drawState("Standby");
}

bool BadUsbScreen::update(uint8_t pressed, uint32_t now) {
    usbWatch_.update(now);
    const uint32_t elapsed = now - armedAt_;
    if (armed_ && elapsed >= kArmWindowMs) {
        armed_ = false;
        usbWatch_.clear();
        drawState("Timed out");
    } else if (armed_ && pressed == kCancelOrBackButton) {
        armed_ = false;
        usbWatch_.clear();
        drawState("Cancelled");
    } else if (armed_ && mode_ == Mode::Passive && pressed == kModeOrInjectButton) {
        armed_ = false;
        if (USBDevice.configured()) {
            runPayload();
        } else {
            drawState("USB unavailable");
        }
    } else if (!armed_ && pressed == kArmButton) {
        armed_ = true;
        armedAt_ = now;
        if (mode_ == Mode::Active) {
            usbWatch_.start(now);
        }
        drawState("");
        displayedSeconds_ = 0;
        drawCountdown(kArmWindowSeconds, 0);
    } else if (!armed_ && pressed == kModeOrInjectButton) {
        mode_ = mode_ == Mode::Passive ? Mode::Active : Mode::Passive;
        drawState("Standby");
    } else if (!armed_ && pressed == kCancelOrBackButton) {
        return true;
    } else if (armed_) {
        if (mode_ == Mode::Active) {
            if (usbWatch_.result() == UsbConnectionWatch::Result::Enumerated) {
                armed_ = false;
                usbWatch_.clear();
                drawState("USB seen (test)");
                return false;
            }
            if (usbWatch_.result() == UsbConnectionWatch::Result::FramesOnly) {
                usbWatch_.start(now);
                drawWaitStatus("REPLUG USB");
            }
            drawAnimation(now, true);
        }
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
    display_.clearWindow(4, 1, 91, 12);
    display_.clearWindow(4, 16, 91, 33);
    display_.clearWindow(4, 51, 91, 12);
    displayedAnimationFrame_ = 0xff;
}

void BadUsbScreen::drawState(const char *message) {
    using namespace TerminalTheme;

    clearStatusArea();

    if (!armed_) {
        const uint16_t modeInk = mode_ == Mode::Passive ? kCyan : kAmber;
        drawText(display_, 4, 2, kCyan, payload_->label);
        drawModeSwitch(modeInk);
        drawStatusIcon();
        drawText(display_, 31, 15, modeInk, mode_ == Mode::Passive ? "PASSIVE" : "ACTIVE",
                 liberationSansNarrow_12ptFontInfo);
        drawText(display_, 31, 30, kMuted, "DISARMED");
        display_.drawRect(4, 43, 3, 3, TSRectangleFilled, modeInk);
        drawText(display_, 10, 39, kMuted, message);
        display_.drawLine(8, 54, 4, 57, kControl);
        display_.drawLine(4, 57, 8, 60, kControl);
        drawText(display_, 12, 52, kControl, "BACK");
        drawText(display_, 72, 52, kAmber, "ARM");
        return;
    }

    display_.drawRect(4, 55, 5, 5, TSRectangleFilled, kCancel);
    drawText(display_, 12, 52, kCancel, "CANCEL");
    if (mode_ == Mode::Active) {
        drawText(display_, 4, 2, kCyan, payload_->label);
        display_.drawRect(4, 16, 23, 18, TSRectangleNoFill, kFrame);
        display_.drawLine(6, 24, 9, 24, kAmber);
        display_.drawRect(9, 21, 6, 7, TSRectangleFilled, kAmber);
        display_.drawRect(15, 22, 4, 5, TSRectangleNoFill, kAmber);
        drawWaitStatus("WAIT USB");
        drawAnimation(millis(), true);
    } else {
        drawText(display_, 4, 2, kCyan, "PASSIVE");
        drawStatusIcon();
        drawText(display_, 31, 16, kAmber, "ARMED", liberationSansNarrow_14ptFontInfo);
        drawText(display_, 50, 2, kGreen, "INJECT");
        display_.drawLine(88, 3, 92, 7, kGreen);
        display_.drawLine(92, 7, 88, 11, kGreen);
    }
}

void BadUsbScreen::drawModeSwitch(uint16_t ink) {
    using namespace TerminalTheme;

    const bool passive = mode_ == Mode::Passive;
    drawText(display_, 76, 2, passive ? ink : kMuted, "P");
    drawText(display_, 87, 2, passive ? kMuted : ink, "A");
    const uint8_t x = passive ? 75 : 86;
    display_.drawLine(x, 12, x + 8, 12, ink);
}

void BadUsbScreen::drawStatusIcon() {
    using namespace TerminalTheme;

    if (armed_) {
        display_.drawLine(15, 17, 25, 33, kAmber);
        display_.drawLine(25, 33, 5, 33, kAmber);
        display_.drawLine(5, 33, 15, 17, kAmber);
        display_.drawLine(15, 22, 15, 27, kAmber);
        display_.drawRect(15, 29, 1, 1, TSRectangleFilled, kAmber);
        return;
    }

    display_.drawLine(6, 17, 24, 17, kGreen);
    display_.drawLine(6, 17, 6, 28, kGreen);
    display_.drawLine(24, 17, 24, 28, kGreen);
    display_.drawLine(6, 28, 15, 36, kGreen);
    display_.drawLine(24, 28, 15, 36, kGreen);
    display_.drawLine(10, 25, 13, 28, kGreen);
    display_.drawLine(13, 28, 20, 21, kGreen);
}

void BadUsbScreen::drawWaitStatus(const char *message) {
    display_.clearWindow(31, 20, 64, 10);
    TerminalTheme::drawText(display_, 31, 20, TerminalTheme::kAmber, message);
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

void BadUsbScreen::drawAnimation(uint32_t now, bool usbWait) {
    const uint8_t active = (now / kSpinnerFrameMs) % kSpinnerDotCount;
    if (active == displayedAnimationFrame_) {
        return;
    }
    displayedAnimationFrame_ = active;
    if (usbWait) {
        for (uint8_t bar = 0; bar < 3; ++bar) {
            const uint8_t x = 20 + 2 * bar;
            display_.drawLine(x, 23 - bar, x, 25 + bar,
                              bar < active ? TerminalTheme::kAmber : kSpentOutline);
        }
        return;
    }
    for (uint8_t dot = 0; dot < kSpinnerDotCount; ++dot) {
        display_.drawRect(kSpinnerDots[dot][0], kSpinnerDots[dot][1], 2, 2, TSRectangleFilled,
                          dot == active ? TerminalTheme::kAmber : kSpentOutline);
    }
}

void BadUsbScreen::drawLoadingProgress(uint8_t percent) {
    drawAnimation(millis(), false);
    const uint8_t width = (kLoadingBarWidth - 2) * percent / 100;
    if (width > 0) {
        display_.drawRect(kLoadingBarX + 1, kLoadingBarY + 1, width, kLoadingBarHeight - 2,
                          TSRectangleFilled, TerminalTheme::kAmber);
    }
}
