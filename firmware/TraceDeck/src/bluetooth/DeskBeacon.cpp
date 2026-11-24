#include "DeskBeacon.h"
#include "../ui/TerminalTheme.h"
#include <STBLE.h>
#include <cstdio>

namespace {
constexpr char Marker[] = "TD-B01";
constexpr uint8_t Advert[] = {
    2, 0x01, 0x06,
    7, 0x09, 'T', 'D', '-', 'B', '0', '1',
    17, 0x07, 0x5E, 0xC6, 0x37, 0x4D, 0x10, 0x64, 0x70, 0x8C,
    0xE4, 0x49, 0x56, 0x52, 0x2B, 0xBA, 0xE4, 0x4E,
};
constexpr uint8_t Response[] = {
    12, 0xFF, 0xFF, 0xFF, 'O', 'S', '=', 'S', 'A', 'M', 'D', '2', '1',
};
static_assert(sizeof(Advert) <= 31 && sizeof(Response) <= 31, "Legacy advertising payload limit");
}

int DeskBeacon::start() {
    stage_ = "Shield version";
    HCI_Init();
    BNRG_SPI_Init();
    BlueNRG_RST();
    delay(100);
    int result = getBlueNRGVersion(&hardware_, &firmware_);
    if (result) return result;

    stage_ = "Random address";
    uint8_t randomBytes[8];
    result = hci_le_rand(randomBytes);
    if (result) return result;
    randomBytes[5] |= 0xC0;
    result = hci_le_set_random_address(randomBytes);
    if (result) return result;

    stage_ = "ADV parameters";
    const tBDAddr unusedPeer = {};
    // 200 ms, 1M legacy advertising, all three advertising channels, no whitelist.
    result = hci_le_set_advertising_parameters(320, 320, ADV_SCAN_IND,
                                             STATIC_RANDOM_ADDR, PUBLIC_ADDR, unusedPeer, 7, 0);
    if (result) return result;
    stage_ = "ADV data";
    result = hci_le_set_advertising_data(sizeof(Advert), Advert);
    if (result) return result;
    stage_ = "Scan response";
    result = hci_le_set_scan_resp_data(sizeof(Response), Response);
    if (result) return result;
    stage_ = "ADV enable";
    result = hci_le_set_advertise_enable(1);
    if (result) return result;
    stage_ = "Advertising";
    return BLE_STATUS_SUCCESS;
}

void DeskBeacon::draw() {
    using namespace TerminalTheme;
    const auto &font = liberationSansNarrow_8ptFontInfo;
    drawFrame(display_);
    drawText(display_, 4, 1, kCyan, "DESK BEACON", font);
    if (status_) {
        char line[20];
        snprintf(line, sizeof(line), "Radio error 0x%02X", status_);
        drawText(display_, 4, 17, kAmber, line, font);
        drawText(display_, 4, 32, kMuted, stage_, font);
    } else {
        drawText(display_, 4, 17, kControl, Marker, font);
        drawText(display_, 4, 32, advertising_ ? kGreen : kAmber,
                 advertising_ ? "Advertising" : "Paused", font);
    }
    const char *controls = !ready_ ? "L1 Retry   L2 Back" : advertising_ ?
                           "L1 Pause  L2 Back" : "L1 Start   L2 Back";
    drawText(display_, 3, 51, kMuted, controls, font);
}

void DeskBeacon::enter() {
    hardware_ = firmware_ = 0;
    status_ = start();
    detachInterrupt(BNRG_SPI_EXTI_PIN);
    ready_ = advertising_ = status_ == BLE_STATUS_SUCCESS;
    ignoreSerialLine_ = false;
    lastPrint_ = millis();
    draw();
}

void DeskBeacon::setAdvertising(bool enabled) {
    if (!ready_ || (enabled == advertising_ && !status_)) return;
    attachInterrupt(BNRG_SPI_EXTI_PIN, HCI_Isr, RISING);
    status_ = hci_le_set_advertise_enable(enabled);
    detachInterrupt(BNRG_SPI_EXTI_PIN);
    if (status_ == BLE_STATUS_SUCCESS) advertising_ = enabled;
    stage_ = status_ ? "ADV toggle error" : (advertising_ ? "Advertising" : "Advertising off");
    draw();
}

bool DeskBeacon::update(uint8_t pressed) {
    if (pressed == TSButtonLowerLeft) {
        detachInterrupt(BNRG_SPI_EXTI_PIN);
        digitalWrite(BNRG_SPI_RESET_PIN, LOW);
        ready_ = advertising_ = false;
        return true;
    }
    if (ready_) HCI_Process();
    if (pressed == TSButtonUpperLeft) {
        if (ready_) setAdvertising(!advertising_);
        else enter();
    }
    if (SerialUSB.available()) {
        const int command = SerialUSB.read();
        if (command == '\n' || command == '\r') ignoreSerialLine_ = false;
        else if (!ignoreSerialLine_ && (command == '0' || command == '1')) setAdvertising(command == '1');
        else ignoreSerialLine_ = true; // Ignore framed commands intended for the audit.
    }
    const uint32_t now = millis();
    if (!SerialUSB || now - lastPrint_ < 1000) return false;
    lastPrint_ = now;
    SerialUSB.print("BEACON|marker="); SerialUSB.print(Marker);
    SerialUSB.print("|stage="); SerialUSB.print(stage_);
    SerialUSB.print("|status="); SerialUSB.print(status_);
    SerialUSB.print("|hw=0x"); SerialUSB.print(hardware_, HEX);
    SerialUSB.print("|fw=0x"); SerialUSB.println(firmware_, HEX);
    return false;
}
