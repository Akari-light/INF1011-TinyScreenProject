#pragma once

#include <stddef.h>
#include <stdint.h>

namespace Ble {

constexpr uint8_t kPayloadSize = 31;

struct Report {
    uint8_t event;
    uint8_t addressType;
    const uint8_t *address;
    const uint8_t *data;
    uint8_t size;
    int8_t rssi;
};

// The HCI event contains a count followed by individual legacy reports.
inline bool readReport(const uint8_t *&data, size_t &remaining, Report &report) {
    if (remaining < 10 || data[8] > kPayloadSize || remaining < 10u + data[8]) return false;
    const uint8_t size = data[8];
    const int value = data[9 + size];
    report = {data[0], data[1], data + 2, data + 9, size, static_cast<int8_t>(value < 128 ? value : value - 256)};
    data += 10 + size;
    remaining -= 10 + size;
    return true;
}

struct AdField {
    uint8_t type;
    const uint8_t *data;
    uint8_t size;
};

inline bool nextField(const uint8_t *&data, uint8_t &remaining, AdField &field) {
    if (remaining == 0 || data[0] == 0 || data[0] >= remaining) return false;
    const uint8_t size = data[0];
    field = {data[1], data + 2, static_cast<uint8_t>(size - 1)};
    data += size + 1;
    remaining -= size + 1;
    return true;
}

inline uint16_t littleEndian16(const uint8_t *data) {
    return data[0] | (uint16_t(data[1]) << 8);
}

}
