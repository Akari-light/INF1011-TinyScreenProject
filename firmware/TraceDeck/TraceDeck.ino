#include <TinyScreen.h>

#include "ButtonReader.h"
#include "HomeScreen.h"
#include "UsbTestScreen.h"

namespace {
enum class Screen { Home, UsbTest };

TinyScreen display(TinyScreenPlus);
ButtonReader buttons(display);
HomeScreen home(display);
UsbTestScreen usbTest(display);
Screen screen = Screen::Home;
}

void setup() {
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
    if (screen == Screen::Home) {
        if (home.openUsbOnPress(pressed)) {
            screen = Screen::UsbTest;
            usbTest.enter();
        }
    } else if (usbTest.update(pressed, now)) {
        screen = Screen::Home;
        home.enter();
    }
}
