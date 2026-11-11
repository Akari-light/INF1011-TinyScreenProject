#include <Keyboard.h>
#include <TinyScreen.h>
#include <stdio.h>

namespace {
constexpr uint32_t kStartupGuardMs = 2000;
constexpr uint32_t kQuietMs = 250;
constexpr uint32_t kRecentFrameMs = 25;
constexpr uint32_t kStableLinkMs = 150;
constexpr uint32_t kWatchMs = 60000;
constexpr uint32_t kDebounceMs = 25;
constexpr uint16_t kCyan = (30u << 11) | (57u << 5) | 6u; // TinyScreen BGR565

enum class Watch { Idle, Waiting, Blocked, Enumerated, FramesOnly, TimedOut };

struct UsbSample {
    bool framesLive;
    bool configured;
    uint8_t address;
};

class UsbConnectionProbe {
public:
    void begin() {
        // Link the same HID descriptor as TraceDeck. Keyboard.begin() only sets the layout.
        Keyboard.begin();
        display_.begin();
        display_.setBitDepth(TSBitDepth16);
        display_.setBrightness(10);
        startedAt_ = lastFrameAt_ = buttonsChangedAt_ = millis();
        lastFrame_ = USB->DEVICE.FNUM.bit.FNUM;
        rawButtons_ = stableButtons_ = display_.getButtons();
        sample_ = readUsb(startedAt_);

        display_.clearScreen();
        drawText(4, 2, kCyan, "USB PROBE");
        display_.drawLine(4, 14, 92, 14, kCyan);
        drawText(4, 52, TS_16b_White, "CLEAR");
        drawText(60, 52, TS_16b_White, "WATCH");
        drawStatus();
    }

    void update(uint32_t now) {
        const UsbSample previous = sample_;
        const Watch previousWatch = watch_;
        const bool previousUnconfigured = sawUnconfigured_;
        sample_ = readUsb(now);
        updateWatch(readPress(now), now);

        if (sample_.framesLive != previous.framesLive || sample_.configured != previous.configured ||
            sample_.address != previous.address || watch_ != previousWatch ||
            sawUnconfigured_ != previousUnconfigured) {
            drawStatus();
        }

        if (SerialUSB.available() && SerialUSB.read() == '?') {
            char line[100];
            snprintf(line, sizeof line, "frames=%s cfg=%u addr=%u watch=%s uncfg=%u uptime_ms=%lu",
                     sample_.framesLive ? "live" : "quiet", static_cast<unsigned>(sample_.configured),
                     static_cast<unsigned>(sample_.address), watchLabel(),
                     static_cast<unsigned>(sawUnconfigured_), static_cast<unsigned long>(now - startedAt_));
            SerialUSB.println(line);
        }
    }

private:
    UsbSample readUsb(uint32_t now) {
        const uint16_t frame = USB->DEVICE.FNUM.bit.FNUM;
        if (frame != lastFrame_) {
            lastFrame_ = frame;
            lastFrameAt_ = now;
            sawFrames_ = true;
        }
        const uint8_t addressRegister = USB->DEVICE.DADD.reg;
        const uint8_t address = (addressRegister & USB_DEVICE_DADD_ADDEN)
            ? addressRegister & USB_DEVICE_DADD_DADD_Msk : 0;
        return {sawFrames_ && now - lastFrameAt_ < kQuietMs, USBDevice.configured(), address};
    }

    uint8_t readPress(uint32_t now) {
        const uint8_t buttons = display_.getButtons();
        if (buttons != rawButtons_) {
            rawButtons_ = buttons;
            buttonsChangedAt_ = now;
        }
        if (rawButtons_ == stableButtons_ || now - buttonsChangedAt_ < kDebounceMs) {
            return 0;
        }
        const uint8_t pressed = stableButtons_ == 0 ? rawButtons_ : 0;
        stableButtons_ = rawButtons_;
        return pressed;
    }

    void updateWatch(uint8_t pressed, uint32_t now) {
        if (pressed == TSButtonLowerLeft) {
            watch_ = Watch::Idle;
            sawUnconfigured_ = false;
        } else if (pressed == TSButtonLowerRight) {
            const bool readyToWatch = now - startedAt_ >= kStartupGuardMs && now - lastFrameAt_ >= kQuietMs;
            watch_ = readyToWatch ? Watch::Waiting : Watch::Blocked;
            watchStartedAt_ = now;
            sawUnconfigured_ = false;
            settling_ = false;
        }
        if (watch_ != Watch::Waiting) {
            return;
        }
        if (now - watchStartedAt_ >= kWatchMs) {
            watch_ = Watch::TimedOut;
            return;
        }

        const bool addressedAndConfigured = sample_.address != 0 && sample_.configured;
        sawUnconfigured_ |= !addressedAndConfigured;
        const bool usable = addressedAndConfigured && sawFrames_ && now - lastFrameAt_ < kRecentFrameMs;
        if (!usable) {
            settling_ = false;
        } else if (!settling_) {
            usableSince_ = now;
            settling_ = true;
        } else if (now - usableSince_ >= kStableLinkMs) {
            watch_ = sawUnconfigured_ ? Watch::Enumerated : Watch::FramesOnly;
        }
    }

    const char *watchLabel() const {
        switch (watch_) {
        case Watch::Idle: return "IDLE";
        case Watch::Waiting: return "WATCHING USB";
        case Watch::Blocked: return "WAIT FOR QUIET";
        case Watch::Enumerated: return "ENUM + LIVE";
        case Watch::FramesOnly: return "LIVE NO ENUM";
        case Watch::TimedOut: return "TIMED OUT";
        }
        return "UNKNOWN";
    }

    void drawStatus() {
        display_.clearWindow(4, 16, 91, 33);
        drawText(4, 16, TS_16b_White, sample_.framesLive ? "FRAMES LIVE" : "FRAMES QUIET");
        char config[19];
        snprintf(config, sizeof config, "CFG %u ADDR %u", static_cast<unsigned>(sample_.configured),
                 static_cast<unsigned>(sample_.address));
        drawText(4, 27, TS_16b_White, config);
        drawText(4, 38, kCyan, watchLabel());
    }

    void drawText(uint8_t x, uint8_t y, uint16_t ink, const char *text) {
        display_.setFont(thinPixel7_10ptFontInfo);
        display_.fontColor(ink, TS_16b_Black);
        display_.setCursor(x, y);
        display_.print(text);
    }

    TinyScreen display_{TinyScreenPlus};
    UsbSample sample_{};
    Watch watch_ = Watch::Idle;
    uint32_t startedAt_ = 0;
    uint32_t lastFrameAt_ = 0;
    uint32_t watchStartedAt_ = 0;
    uint32_t usableSince_ = 0;
    uint32_t buttonsChangedAt_ = 0;
    uint16_t lastFrame_ = 0;
    uint8_t rawButtons_ = 0;
    uint8_t stableButtons_ = 0;
    bool sawFrames_ = false;
    bool sawUnconfigured_ = false;
    bool settling_ = false;
};

UsbConnectionProbe probe;
}

void setup() {
    probe.begin();
}

void loop() {
    probe.update(millis());
}
