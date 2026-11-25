#pragma once

#include "AdvertisingData.h"

namespace Ble {

constexpr uint32_t kRecentMs = 10000;

struct Packet {
    uint8_t bytes[kPayloadSize];
    uint8_t size;
    uint8_t event;
    uint32_t seenAt;
    bool seen;
};

struct Observation {
    uint8_t address[6];
    uint8_t type;
    uint16_t number;
    uint32_t seenAt;
    int8_t rssi;
    char name[30];
    uint16_t company;
    bool hasCompany;
    bool lab;
    char platform[25];
    Packet advertisement;
    Packet response;

    bool named() const { return name[0] != '\0'; }
};

struct RecentAddresses {
    uint8_t named = 0;
    uint8_t unnamed = 0;
    int8_t strongestUnnamed = 127;
    uint32_t latestUnnamedAge = kRecentMs;

    uint8_t total() const { return named + unnamed; }
};

class Scanner {
public:
    void begin();
    void end();
    void update();
    void receive(const uint8_t *packet);

    bool running() const { return running_; }
    int status() const { return status_; }
    const char *stage() const { return stage_; }
    uint32_t reports() const { return reports_; }
    uint32_t invalidEvents() const { return invalid_; }
    bool saturated(uint32_t now) const { return saturated_ && now - overflowAt_ < kRecentMs; }
    RecentAddresses recent(uint32_t now) const;
    uint8_t namedList(uint16_t *numbers, uint32_t now, uint16_t selected) const;
    const Observation *find(uint16_t number) const;
    const Observation *deskBeacon(uint32_t now) const;
    void protect(uint16_t number) { protectedNumber_ = number; }

    static constexpr uint8_t kCapacity = 80;

private:
    void startObservation();
    void observe(const Report &report, uint32_t now);

    Observation entries_[kCapacity] = {};
    uint16_t nextNumber_ = 0;
    uint16_t protectedNumber_ = 0;
    uint32_t restartAt_ = 0, reports_ = 0, invalid_ = 0, overflowAt_ = 0;
    int status_ = 0;
    bool running_ = false, observerReady_ = false, saturated_ = false;
    const char *stage_ = "Ready";
};

const char *addressTypeName(uint8_t type);
const char *eventName(uint8_t event);
void formatAddress(char *out, size_t size, const Observation &entry);

}
