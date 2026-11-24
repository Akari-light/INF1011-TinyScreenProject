#include <TinyScreen.h>

#include "src/badusb/BadUsbPayloadMenu.h"
#include "src/badusb/BadUsbScreen.h"
#include "src/bluetooth/BluetoothMenu.h"
#include "src/input/ButtonReader.h"
#include "src/ui/HomeScreen.h"

namespace {
enum class Screen { Home, BadUsbPayloadMenu, BadUsbExecution, Bluetooth };

TinyScreen display(TinyScreenPlus);
ButtonReader buttons(display);
HomeScreen home(display);
BadUsbPayloadMenu payloadMenu(display);
BadUsbScreen badUsb(display);
BluetoothMenu bluetooth(display);
Screen screen = Screen::Home;
}

void setup() {
    SerialUSB.begin(115200);
    display.begin();
    display.setBitDepth(TSBitDepth16);
    display.setBrightness(10);
    buttons.begin(millis());
    home.enter();
}

void loop() {
    const uint32_t now = millis();
    const uint8_t pressed = buttons.readPress(now);
    // Only the current screen receives the press. The reader keeps its held state.
    switch (screen) {
    case Screen::Home: {
        const HomeScreen::Action action = home.update(pressed);
        if (action == HomeScreen::Action::BadUsb) {
            screen = Screen::BadUsbPayloadMenu;
            payloadMenu.enter();
        } else if (action == HomeScreen::Action::Bluetooth) {
            screen = Screen::Bluetooth;
            bluetooth.enter();
        }
        break;
    }
    case Screen::BadUsbPayloadMenu: {
        const BadUsbPayloadMenu::Action action = payloadMenu.update(pressed);
        if (action == BadUsbPayloadMenu::Action::Open) {
            screen = Screen::BadUsbExecution;
            badUsb.enter(payloadMenu.selectedPayload());
        } else if (action == BadUsbPayloadMenu::Action::Back) {
            screen = Screen::Home;
            home.enter();
        }
        break;
    }
    case Screen::BadUsbExecution:
        if (badUsb.update(pressed, now)) {
            screen = Screen::BadUsbPayloadMenu;
            payloadMenu.enter();
        }
        break;
    case Screen::Bluetooth:
        if (bluetooth.update(pressed)) {
            screen = Screen::Home;
            home.enter();
        }
        break;
    }
}
