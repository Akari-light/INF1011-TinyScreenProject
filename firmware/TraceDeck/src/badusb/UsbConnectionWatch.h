#pragma once

#include <Arduino.h>

class UsbConnectionWatch {
public:
    enum class Result { Idle, WaitingForQuiet, WaitingForLink, Enumerated, FramesOnly };

    void reset(uint32_t now) {
        lastFrame_ = USB->DEVICE.FNUM.bit.FNUM;
        lastFrameAt_ = now;
        sawFrames_ = false;
        clear();
    }

    void start(uint32_t now) {
        clear();
        result_ = Result::WaitingForQuiet;
        update(now);
    }

    void clear() {
        result_ = Result::Idle;
        sawUnconfigured_ = false;
        settling_ = false;
    }

    void update(uint32_t now) {
        const uint16_t frame = USB->DEVICE.FNUM.bit.FNUM;
        if (frame != lastFrame_) {
            lastFrame_ = frame;
            lastFrameAt_ = now;
            sawFrames_ = true;
        }

        const uint32_t frameAge = now - lastFrameAt_;
        if (result_ == Result::WaitingForQuiet && frameAge >= kQuietMs) {
            result_ = Result::WaitingForLink;
        }
        if (result_ != Result::WaitingForLink) {
            return;
        }

        const uint8_t address = USB->DEVICE.DADD.reg;
        const bool configured = USBDevice.configured() && (address & USB_DEVICE_DADD_ADDEN) &&
                                (address & USB_DEVICE_DADD_DADD_Msk);
        sawUnconfigured_ |= !configured;
        const bool usable = configured && sawFrames_ && frameAge < kRecentFrameMs;
        if (!usable) {
            settling_ = false;
        } else if (!settling_) {
            usableSince_ = now;
            settling_ = true;
        } else if (now - usableSince_ >= kStableLinkMs) {
            // Re-enumeration can also occur during wake. This is an observation only.
            result_ = sawUnconfigured_ ? Result::Enumerated : Result::FramesOnly;
        }
    }

    Result result() const { return result_; }

private:
    static constexpr uint32_t kQuietMs = 250;
    static constexpr uint32_t kRecentFrameMs = 25;
    static constexpr uint32_t kStableLinkMs = 150;

    Result result_ = Result::Idle;
    uint32_t lastFrameAt_ = 0;
    uint32_t usableSince_ = 0;
    uint16_t lastFrame_ = 0;
    bool sawFrames_ = false;
    bool sawUnconfigured_ = false;
    bool settling_ = false;
};
