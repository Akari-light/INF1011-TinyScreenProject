#include "BleNearbyScreen.h"

#include <cstdio>
#include <cstring>

#include "../ui/TerminalTheme.h"

namespace {
using namespace TerminalTheme;
constexpr uint8_t kRows = 3;
constexpr uint8_t kRowStart = 16;
constexpr uint8_t kRowStep = 11;
constexpr uint8_t kTextX = 13;
const FONT_INFO &kBodyFont = liberationSansNarrow_8ptFontInfo;

void clip(TinyScreen &display, char *text, uint8_t width) {
    display.setFont(kBodyFont);
    uint16_t total = 0;
    for (char *next = text; *next; ++next) {
        char glyph[] = {*next, '\0'};
        total += display.getPrintWidth(glyph);
        if (total > width) {
            *next = '\0';
            return;
        }
    }
}

const char *companyName(uint16_t company) {
    if (company == 0x004C) return "Apple";
    if (company == 0x0075) return "Samsung";
    return nullptr;
}

void formatSignal(char *out, size_t size, const char *label, int8_t rssi) {
    if (rssi == 127) snprintf(out, size, "%s --", label);
    else snprintf(out, size, "%s %d dBm", label, rssi);
}

void appendHex(char *out, const Ble::Packet &packet) {
    for (uint8_t i = 0; i < packet.size; ++i) snprintf(out + 2 * i, 3, "%02X", packet.bytes[i]);
}
}

void BleNearbyScreen::enter(bool audit) {
    auditing_ = audit;
    if (audit) {
        audit_.enter();
        scanner_.begin();
        return;
    }
    for (auto &line : lines_) line = {};
    details_ = false;
    forceDraw_ = true;
    for (auto &line : lines_) line.ink = kMuted;
    strcpy(lines_[0].text, "Starting scan");
    lines_[0].signal[0] = lines_[1].text[0] = lines_[2].text[0] = '\0';
    draw("BLE NEARBY", "", "");
    forceDraw_ = true;
    selected_ = 0;
    details_ = explicitSelection_ = false;
    lastDraw_ = lastLog_ = 0;
    scanner_.begin();
}

bool BleNearbyScreen::update(uint8_t pressed) {
    scanner_.update();
    if (auditing_) {
        if (!audit_.update(display_, scanner_, pressed)) return false;
        scanner_.end();
        return true;
    }
    const uint32_t now = millis();
    if (pressed == TSButtonLowerLeft) {
        if (!details_) {
            scanner_.end();
            return true;
        }
        details_ = false;
        forceDraw_ = true;
    }
    if (!forceDraw_ && !pressed && now - lastDraw_ < 250) return false;
    lastDraw_ = now;

    const Ble::RecentAddresses recent = scanner_.recent(now);
    for (Line &line : lines_) line = {};
    if (details_) {
        const auto *entry = scanner_.find(selected_);
        if (pressed == TSButtonUpperLeft && entry && entry->lab && !strcmp(entry->name, "TD-B01")) {
            auditing_ = true;
            audit_.enter();
            return false;
        }
        detail(now, entry, recent);
    } else {
        uint16_t ordered[Ble::Scanner::kCapacity + 1];
        uint8_t count = scanner_.namedList(ordered, now, selected_);
        // Address zero is reserved for the single expandable summary row.
        if (recent.unnamed) ordered[count++] = 0;
        uint8_t index = 0;
        while (index < count && ordered[index] != selected_) ++index;
        if (index == count || !explicitSelection_) index = 0;

        if (count && (pressed == TSButtonUpperRight || pressed == TSButtonLowerRight)) {
            explicitSelection_ = true;
            index = (index + (pressed == TSButtonUpperRight ? count - 1 : 1)) % count;
        }
        selected_ = count ? ordered[index] : 0;
        scanner_.protect(selected_);
        if (count && pressed == TSButtonUpperLeft) {
            details_ = explicitSelection_ = true;
            forceDraw_ = true;
            detail(now, scanner_.find(selected_), recent);
        } else {
            list(now, ordered, count, index, recent);
        }
    }

    if (now - lastLog_ >= 1000) {
        lastLog_ = now;
        if (SerialUSB) log(now, recent);
    }
    return false;
}

void BleNearbyScreen::list(uint32_t now, const uint16_t *ordered, uint8_t count, uint8_t index,
                           Ble::RecentAddresses recent) {
    char footer[24];
    if (scanner_.running()) {
        snprintf(footer, sizeof(footer), "LIVE %u%s/10s", recent.total(),
                 scanner_.saturated(now) ? "+" : "");
    } else {
        strcpy(footer, "SCAN OFF");
    }
    if (count == 0) {
        lines_[0].ink = scanner_.running() ? kMuted : kAmber;
        snprintf(lines_[0].text, sizeof(lines_[0].text), "%s",
                 scanner_.running() ? "Waiting for adverts" : scanner_.stage());
        if (!scanner_.running()) {
            lines_[1].ink = kMuted;
            snprintf(lines_[1].text, sizeof(lines_[1].text), "Radio status 0x%02X", scanner_.status());
        }
    }
    const uint8_t top = index < kRows ? 0 : index - kRows + 1;
    for (uint8_t i = 0; i < kRows && top + i < count; ++i) {
        const uint16_t number = ordered[top + i];
        Line &line = lines_[i];
        line.selected = number == selected_;
        if (number == 0) {
            line.ink = line.selected ? kGreen : kMuted;
            snprintf(line.text, sizeof(line.text), "Unnamed %u", recent.unnamed);
            continue;
        }
        const auto &entry = *scanner_.find(number);
        const bool fresh = now - entry.seenAt < Ble::kRecentMs;
        line.ink = fresh ? (line.selected ? kGreen : kMuted) : kAmber;
        snprintf(line.text, sizeof(line.text), "%s", entry.name);
        if (!fresh) strcpy(line.signal, "stale");
        else if (entry.rssi == 127) strcpy(line.signal, "--");
        else snprintf(line.signal, sizeof(line.signal), "%d", entry.rssi);
    }
    if (count == 1 && ordered[0] == 0) {
        lines_[1].ink = kMuted;
        strcpy(lines_[1].text, "No names yet");
    }
    draw("BLE NEARBY", count ? "OPEN" : "", footer, count > 1);
}

void BleNearbyScreen::detail(uint32_t now, const Ble::Observation *entry, Ble::RecentAddresses recent) {
    for (Line &line : lines_) line.ink = kMuted;
    const uint32_t age = entry == nullptr ? recent.latestUnnamedAge : now - entry->seenAt;
    const bool fresh = age < Ble::kRecentMs;
    lines_[0].ink = fresh ? kGreen : kAmber;
    char secondary[24] = {};
    if (entry == nullptr) {
        snprintf(lines_[0].text, sizeof(lines_[0].text), "%u addresses/10s", recent.unnamed);
        formatSignal(lines_[1].text, sizeof(lines_[1].text), "Max", recent.strongestUnnamed);
    } else {
        snprintf(lines_[0].text, sizeof(lines_[0].text), "%s", entry->named() ? entry->name : "Name unavailable");
        formatSignal(lines_[1].text, sizeof(lines_[1].text), "Signal", entry->rssi);
        const char *company = entry->hasCompany ? companyName(entry->company) : nullptr;
        if (company) snprintf(secondary, sizeof(secondary), "%s data", company);
    }
    snprintf(lines_[2].text, sizeof(lines_[2].text), "Last seen %lus%s",
             static_cast<unsigned long>(fresh ? age / 1000 : Ble::kRecentMs / 1000), fresh ? "" : "+");
    const bool desk = entry && entry->lab && !strcmp(entry->name, "TD-B01");
    draw(desk ? "WALK-AWAY" : entry == nullptr ? "UNNAMED" : "DEVICE", desk ? "OPEN" : "", secondary);
}

void BleNearbyScreen::arrows() {
    display_.drawLine(87, 7, 90, 4, kControl);
    display_.drawLine(90, 4, 93, 7, kControl);
    display_.drawLine(87, 56, 90, 59, kControl);
    display_.drawLine(90, 59, 93, 56, kControl);
}

void BleNearbyScreen::draw(const char *title, const char *action, const char *footer, bool scrollable) {
    if (forceDraw_) {
        drawFrame(display_);
        drawControl(display_, 4, 54, kControl, "BACK");
    }
    if (forceDraw_ || previousScrollable_ != scrollable) {
        display_.clearWindow(87, 3, 7, 8);
        display_.clearWindow(87, 53, 7, 8);
        if (scrollable) arrows();
        previousScrollable_ = scrollable;
    }
    if (forceDraw_ || strcmp(title, previousTitle_) || strcmp(action, previousAction_)) {
        display_.clearWindow(3, 1, 84, 12);
        if (*action) drawControl(display_, 4, 3, kControl, action);
        drawText(display_, *action ? 29 : 4, 1, kCyan, title, kBodyFont);
        strcpy(previousTitle_, title);
        strcpy(previousAction_, action);
    }
    if (forceDraw_ || strcmp(footer, previousFooter_)) {
        display_.clearWindow(28, 52, 59, 11);
        drawText(display_, 28, 52, kMuted, footer, kBodyFont);
        strcpy(previousFooter_, footer);
    }
    for (uint8_t i = 0; i < kRows; ++i) {
        Line &line = lines_[i];
        const Line &before = previous_[i];
        clip(display_, line.text, details_ ? 88 : (line.signal[0] ? 53 : 79));
        if (!forceDraw_ && line.ink == before.ink && line.selected == before.selected &&
            strcmp(line.text, before.text) == 0 && strcmp(line.signal, before.signal) == 0) continue;
        const uint8_t y = kRowStart + i * kRowStep;
        display_.clearWindow(4, y, 89, kRowStep);
        if (line.selected) {
            display_.drawLine(5, y + 2, 9, y + 5, kCyan);
            display_.drawLine(9, y + 5, 5, y + 8, kCyan);
        }
        drawText(display_, details_ ? 4 : kTextX, y, line.ink, line.text, kBodyFont);
        if (line.signal[0]) {
            const uint8_t x = 92 - display_.getPrintWidth(line.signal);
            drawText(display_, x, y, line.ink, line.signal, kBodyFont);
        }
        previous_[i] = line;
    }
    forceDraw_ = false;
}

void BleNearbyScreen::log(uint32_t now, Ble::RecentAddresses recent) {
    const Ble::Observation *entry = scanner_.find(selected_);
    char message[180];
    snprintf(message, sizeof(message),
             "\nBLE|ms=%lu|%s|status=%d|recent=%u|named=%u|unnamed=%u|reports=%lu|invalid=%lu|ui=%s|slot=%u\r\n",
             static_cast<unsigned long>(now), scanner_.stage(), scanner_.status(), recent.total(),
             recent.named, recent.unnamed,
             static_cast<unsigned long>(scanner_.reports()), static_cast<unsigned long>(scanner_.invalidEvents()),
             details_ ? (entry ? "detail" : "unnamed") : "list", selected_);
    SerialUSB.print(message);
    if (entry == nullptr || !entry->lab) return;
    char address[18], adv[63] = {}, response[63] = {};
    Ble::formatAddress(address, sizeof(address), *entry);
    appendHex(adv, entry->advertisement);
    appendHex(response, entry->response);
    char record[420];
    snprintf(record, sizeof(record),
             "\nBLE-LAB|name=%s|address=%s|type=%s|rssi=%d|age_ms=%lu"
             "|adv_ms=%lu|adv_event=%s|adv=%s|rsp_ms=%lu|rsp_event=%s|rsp=%s|platform_self_reported=%s\r\n",
             entry->name, address, Ble::addressTypeName(entry->type), entry->rssi,
             static_cast<unsigned long>(now - entry->seenAt), static_cast<unsigned long>(entry->advertisement.seenAt),
             entry->advertisement.seen ? Ble::eventName(entry->advertisement.event) : "",
             adv, static_cast<unsigned long>(entry->response.seenAt),
             entry->response.seen ? Ble::eventName(entry->response.event) : "", response, entry->platform);
    SerialUSB.print(record);
}
