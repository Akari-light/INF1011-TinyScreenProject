#include "BleScanner.h"

#include <STBLE.h>
#include <cstdio>
#include <cstring>

// STBLE 0.1.0 exports this spelling, despite the IDB05A1 suffix in its header.
extern "C" tBleStatus aci_gap_start_observation_procedure(
    uint16_t interval, uint16_t window, uint8_t type, uint8_t addressType, uint8_t duplicates);

namespace {
Ble::Scanner *activeScanner = nullptr;
constexpr uint16_t kTestCompany = 0xFFFF;
constexpr const char *kMarkers[] = {"TD-PHONE", "TD-B01"};
constexpr uint32_t kListHoldMs = 30000;

void copyAscii(char *out, size_t capacity, const uint8_t *bytes, size_t count) {
    const size_t length = min(capacity - 1, count);
    for (size_t i = 0; i < length; ++i) {
        out[i] = bytes[i] >= ' ' && bytes[i] <= '~' && bytes[i] != '|' ? bytes[i] : '?';
    }
    out[length] = '\0';
}

bool isMarker(const uint8_t *data, size_t size, bool allowsSuffix) {
    for (const char *marker : kMarkers) {
        const size_t length = strlen(marker);
        if (size >= length && (size == length || (allowsSuffix && data[length] == ';')) &&
            memcmp(data, marker, length) == 0) return true;
    }
    return false;
}

void copyName(Ble::Observation &entry, const Ble::AdField &field) {
    const uint8_t *name = field.data;
    const auto *end = static_cast<const uint8_t *>(memchr(name, '\0', field.size));
    if (end == nullptr) end = name + field.size;
    while (name < end && *name == ' ') ++name;
    while (name < end && end[-1] == ' ') --end;
    if (name < end) copyAscii(entry.name, sizeof(entry.name), name, end - name);
}

void decodeFields(const Ble::Report &report, Ble::Observation &entry) {
    const uint8_t *data = report.data;
    uint8_t remaining = report.size;
    Ble::AdField field;
    while (Ble::nextField(data, remaining, field)) {
        if (field.type == 0x09 || (field.type == 0x08 && !entry.named())) copyName(entry, field);
        if ((field.type == 0x08 || field.type == 0x09) && isMarker(field.data, field.size, false)) {
            entry.lab = true;
        }
        if (field.type != 0xFF || field.size < 2) continue;
        entry.hasCompany = true;
        entry.company = Ble::littleEndian16(field.data);
        if (entry.company != kTestCompany) continue;
        const uint8_t *text = field.data + 2;
        size_t count = field.size - 2;
        if (isMarker(text, count, true)) {
            entry.lab = true;
            if (entry.name[0] == '\0') {
                const auto *split = static_cast<const uint8_t *>(memchr(text, ';', count));
                copyAscii(entry.name, sizeof(entry.name), text, split ? size_t(split - text) : count);
            }
            const auto *split = static_cast<const uint8_t *>(memchr(text, ';', count));
            if (split == nullptr) continue;
            count -= size_t(split + 1 - text);
            text = split + 1;
        }
        if (entry.lab && count > 3 && memcmp(text, "OS=", 3) == 0) {
            copyAscii(entry.platform, sizeof(entry.platform), text + 3, count - 3);
        }
    }
}
}

extern "C" void HCI_Event_CB(void *packet) {
    if (activeScanner != nullptr) activeScanner->receive(static_cast<const uint8_t *>(packet));
}

namespace Ble {

void Scanner::begin() {
    memset(entries_, 0, sizeof(entries_));
    nextNumber_ = protectedNumber_ = firmware_ = hardware_ = 0;
    restartAt_ = reports_ = invalid_ = overflowAt_ = 0;
    observerReady_ = saturated_ = false;
    activeScanner = this;
    stage_ = "Shield version";
    HCI_Init();
    BNRG_SPI_Init();
    BlueNRG_RST();
    delay(100);
    status_ = getBlueNRGVersion(&hardware_, &firmware_);
    if (status_ == BLE_STATUS_SUCCESS) {
        stage_ = "GATT setup";
        status_ = aci_gatt_init();
    }
    if (status_ == BLE_STATUS_SUCCESS) {
        uint16_t service, name, appearance;
        observerReady_ = aci_gap_init_IDB05A1(
            GAP_OBSERVER_ROLE_IDB05A1, 0, 7, &service, &name, &appearance) == BLE_STATUS_SUCCESS;
        stage_ = "Scanner address";
        uint8_t random[8];
        status_ = hci_le_rand(random);
        if (status_ == BLE_STATUS_SUCCESS) {
            random[5] |= 0xC0;
            status_ = hci_le_set_random_address(random);
        }
    }
    if (status_ == BLE_STATUS_SUCCESS) startObservation();
    detachInterrupt(BNRG_SPI_EXTI_PIN);
}

void Scanner::startObservation() {
    constexpr uint16_t interval = 96, window = 72;
    attachInterrupt(BNRG_SPI_EXTI_PIN, HCI_Isr, RISING);
    stage_ = "Scan start";
    status_ = observerReady_ ? aci_gap_start_observation_procedure(
        interval, window, ACTIVE_SCAN, STATIC_RANDOM_ADDR, 0) : BLE_STATUS_ERROR;
    if (status_ != BLE_STATUS_SUCCESS) {
        status_ = hci_le_set_scan_parameters(ACTIVE_SCAN, interval, window, STATIC_RANDOM_ADDR, 0);
        if (status_ == BLE_STATUS_SUCCESS) status_ = hci_le_set_scan_enable(1, 0);
    }
    // The SAMD interrupt-lock stubs in STBLE 0.1.0 are no-ops. Poll the queue
    // from HCI_Process during reception so it has a single consumer.
    detachInterrupt(BNRG_SPI_EXTI_PIN);
    running_ = status_ == BLE_STATUS_SUCCESS;
    if (running_) stage_ = "Scanning";
}

void Scanner::end() {
    detachInterrupt(BNRG_SPI_EXTI_PIN);
    digitalWrite(BNRG_SPI_RESET_PIN, LOW);
    activeScanner = nullptr;
    running_ = false;
    restartAt_ = 0;
}

void Scanner::update() {
    if (running_) HCI_Process();
    if (restartAt_ != 0 && static_cast<int32_t>(millis() - restartAt_) >= 0) {
        restartAt_ = 0;
        startObservation();
    }
}

void Scanner::receive(const uint8_t *packet) {
    if (packet[0] != HCI_EVENT_PKT) return;
    if (packet[1] == EVT_VENDOR && packet[2] >= 4 &&
        littleEndian16(packet + 3) == EVT_BLUE_GAP_PROCEDURE_COMPLETE &&
        packet[5] == GAP_OBSERVATION_PROC_IDB05A1) {
        running_ = false;
        stage_ = "Restarting";
        restartAt_ = millis() + 100;
    }
    if (packet[1] != EVT_LE_META_EVENT || packet[2] < 2 || packet[3] != EVT_LE_ADVERTISING_REPORT) return;
    const uint8_t *data = packet + 5;
    size_t remaining = packet[2] - 2;
    const uint32_t now = millis();
    for (uint8_t i = 0; i < packet[4]; ++i) {
        Report report;
        if (!readReport(data, remaining, report)) {
            ++invalid_;
            return;
        }
        observe(report, now);
    }
    if (remaining != 0 || packet[4] == 0) ++invalid_;
}

void Scanner::observe(const Report &report, uint32_t now) {
    ++reports_;
    Observation *available = nullptr;
    Observation *entry = nullptr;
    uint32_t oldest = kRecentMs - 1;
    for (Observation &candidate : entries_) {
        if (candidate.number != 0 && candidate.type == report.addressType &&
            memcmp(candidate.address, report.address, 6) == 0) {
            entry = &candidate;
            break;
        }
        if (candidate.number == 0) {
            available = &candidate;
            oldest = UINT32_MAX;
        } else if (candidate.number != protectedNumber_ && now - candidate.seenAt > oldest) {
            available = &candidate;
            oldest = now - candidate.seenAt;
        }
    }
    if (entry == nullptr) {
        if (available == nullptr) {
            saturated_ = true;
            overflowAt_ = now;
            return;
        }
        entry = available;
        *entry = {};
        do {
            ++nextNumber_;
        } while (nextNumber_ == 0 || find(nextNumber_) != nullptr);
        entry->number = nextNumber_;
        entry->type = report.addressType;
        memcpy(entry->address, report.address, 6);
    } else if (now - entry->seenAt >= kRecentMs) {
        entry->lab = false;
        entry->hasCompany = false;
        entry->name[0] = entry->platform[0] = '\0';
        entry->advertisement = {};
        entry->response = {};
    }
    decodeFields(report, *entry);
    entry->seenAt = now;
    entry->rssi = report.rssi;
    if (entry->lab) {
        Packet &target = report.event == SCAN_RSP ? entry->response : entry->advertisement;
        memcpy(target.bytes, report.data, report.size);
        target.size = report.size;
        target.seenAt = now;
        target.event = report.event;
        target.seen = true;
    }
}

const Observation *Scanner::find(uint16_t number) const {
    if (number == 0) return nullptr;
    for (const Observation &entry : entries_) {
        if (entry.number == number) return &entry;
    }
    return nullptr;
}

RecentAddresses Scanner::recent(uint32_t now) const {
    RecentAddresses result;
    for (const Observation &entry : entries_) {
        const uint32_t age = now - entry.seenAt;
        if (entry.number == 0 || age >= kRecentMs) continue;
        if (entry.named()) {
            ++result.named;
            continue;
        }
        ++result.unnamed;
        if (age < result.latestUnnamedAge) result.latestUnnamedAge = age;
        if (entry.rssi != 127 && (result.strongestUnnamed == 127 || entry.rssi > result.strongestUnnamed)) {
            result.strongestUnnamed = entry.rssi;
        }
    }
    return result;
}

uint8_t Scanner::namedList(uint16_t *numbers, uint32_t now, uint16_t selected) const {
    const Observation *orderedEntries[kCapacity];
    uint8_t count = 0;
    for (const Observation &entry : entries_) {
        if (entry.named() && (entry.number == selected || now - entry.seenAt < kListHoldMs)) {
            orderedEntries[count++] = &entry;
        }
    }
    // Keep each priority stable while signal strength changes.
    const auto priority = [now](const Observation &entry) {
        return (now - entry.seenAt < kRecentMs ? 0 : 2) + (entry.lab ? 0 : 1);
    };
    for (uint8_t i = 1; i < count; ++i) {
        const Observation *candidate = orderedEntries[i];
        uint8_t j = i;
        while (j != 0) {
            const Observation *previous = orderedEntries[j - 1];
            const int rank = priority(*candidate);
            const int previousRank = priority(*previous);
            if (rank > previousRank || (rank == previousRank && candidate->number > previous->number)) break;
            orderedEntries[j] = previous;
            --j;
        }
        orderedEntries[j] = candidate;
    }
    for (uint8_t i = 0; i < count; ++i) numbers[i] = orderedEntries[i]->number;
    return count;
}

const char *addressTypeName(uint8_t type) {
    if (type == PUBLIC_ADDR) return "Public";
    if (type == STATIC_RANDOM_ADDR) return "Random";
    return "Unknown";
}

const char *eventName(uint8_t event) {
    switch (event) {
    case ADV_IND: return "ADV_IND";
    case ADV_DIRECT_IND: return "ADV_DIRECT_IND";
    case ADV_SCAN_IND: return "ADV_SCAN_IND";
    case ADV_NONCONN_IND: return "ADV_NONCONN_IND";
    case SCAN_RSP: return "SCAN_RSP";
    default: return "Unknown event";
    }
}

void formatAddress(char *out, size_t size, const Observation &entry) {
    const uint8_t *b = entry.address;
    snprintf(out, size, "%02X:%02X:%02X:%02X:%02X:%02X", b[5], b[4], b[3], b[2], b[1], b[0]);
}

}
