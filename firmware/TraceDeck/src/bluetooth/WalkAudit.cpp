#include "WalkAudit.h"
#include "../ui/TerminalTheme.h"
#include <cstdio>
#include <cstring>

void WalkAudit::enter() {
    used_ = 0;
    overflow_ = false;
    nextDraw_ = 0;
    paintedState_ = -1;
    confirmEnd_ = false;
    if (!trial_[0]) {
        imported_ = false;
        haveBeacon_ = false;
    }
}

// Every reply echoes the request token so a delayed line cannot become a clock sample.
void WalkAudit::command(char *line, uint32_t now, bool fresh) {
    char verb[8], id[33], nonce[9], extra;
    if (sscanf(line, "WA1 %7s %32[0123456789abcdef] %8[0123456789abcdef] %c",
               verb, id, nonce, &extra) != 3 || strlen(id) != 32) return;
    char reply[98];
    const bool same = strcmp(id, trial_) == 0;
    const char *state = "UNKNOWN";
    if (strcmp(verb, "PING") == 0) {
        state = "CLOCK";
    } else if (strcmp(verb, "ARM") == 0 && !trial_[0]) {
        if (fresh) {
            strcpy(trial_, id);
            marked_ = imported_ = false;
            state = "ARMED";
        } else state = "WAIT_BEACON";
    } else if (!same) {
        state = trial_[0] ? "BUSY" : "NO_TRIAL";
    } else if (strcmp(verb, "READ") == 0 || strcmp(verb, "ARM") == 0) {
        state = marked_ ? "MARKED" : "ARMED";
    } else if (strcmp(verb, "ACK") == 0 && marked_) {
        trial_[0] = 0;
        marked_ = false;
        imported_ = true;
        state = "SAVED";
    }
    snprintf(reply, sizeof(reply), "WA1 %s %s %s %lu %lu %lu %d", id, nonce, state,
             (unsigned long)now, (unsigned long)markedAt_, (unsigned long)markerAge_, markerRssi_);
    SerialUSB.println(reply);
}

bool WalkAudit::update(TinyScreen &display, Ble::Scanner &scanner, uint8_t press) {
    using namespace TerminalTheme;
    const uint32_t now = millis();
    if (const auto *beacon = scanner.deskBeacon(now)) {
        haveBeacon_ = true;
        seenAt_ = beacon->seenAt;
        rssi_ = beacon->rssi;
    }
    const uint32_t age = now - seenAt_;
    const bool fresh = scanner.running() && haveBeacon_ && age < Ble::kRecentMs;
    for (unsigned n = 0; n < sizeof(input_) && SerialUSB.available(); ++n) {
        const int c = SerialUSB.read();
        if (c == '\n') {
            input_[used_] = 0;
            if (!overflow_) command(input_, now, fresh);
            used_ = 0;
            overflow_ = false;
        } else if (c != '\r') {
            if (used_ + 1u < sizeof(input_)) input_[used_++] = char(c);
            else overflow_ = true;
        }
    }
    if (confirmEnd_) {
        if (press == TSButtonUpperLeft) {
            trial_[0] = 0;
            marked_ = imported_ = false;
        }
        if (press == TSButtonUpperLeft || press == TSButtonLowerLeft) confirmEnd_ = false;
    } else if (press == TSButtonLowerLeft && trial_[0]) {
        confirmEnd_ = true;
    } else if (press == TSButtonLowerLeft) return true;
    else if (press == TSButtonUpperLeft && trial_[0] && !marked_) {
        markedAt_ = now;
        markerAge_ = age;
        markerRssi_ = rssi_;
        marked_ = true;
        nextDraw_ = 0;
    }
    if (!press && nextDraw_ && int32_t(now - nextDraw_) < 0) return false;
    nextDraw_ = now + 500;
    const auto &font = liberationSansNarrow_8ptFontInfo;
    char radio[28];
    if (!scanner.running()) snprintf(radio, sizeof(radio), "Radio offline");
    else if (!haveBeacon_) snprintf(radio, sizeof(radio), "Finding TD-B01");
    else if (age >= 100000) snprintf(radio, sizeof(radio), "Beacon >99s ago");
    else snprintf(radio, sizeof(radio), "Beacon %lus ago", (unsigned long)(age / 1000));
    const int8_t state = confirmEnd_ ? 4 : marked_ ? 3 : trial_[0] ? 2 : imported_ ? 1 : 0;
    if (paintedState_ != state) {
        drawFrame(display);
        drawText(display, 4, 1, kCyan, "WALK-AWAY", font);
        drawText(display, 4, 17, marked_ || imported_ ? kGreen : kAmber,
                 confirmEnd_ ? "Discard trial?" : marked_ ? "Doorway marked" :
                 trial_[0] ? "Ready to mark" : imported_ ? "Trial saved" : "Dock to arm", font);
        drawText(display, 4, 51, kMuted, confirmEnd_ ? "L1 Yes   L2 Keep" :
                 marked_ ? "Return to dock" : trial_[0] ? "L1  Out of sight" : "L2  Back", font);
    }
    if (paintedState_ != state || strcmp(paintedRadio_, radio)) {
        display.clearWindow(3, 30, 90, 15);
        drawText(display, 4, 32, kControl, radio, font);
        strcpy(paintedRadio_, radio);
    }
    paintedState_ = state;
    return false;
}
