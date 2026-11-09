#include <TinyScreen.h>

#include "BadUsbScreen.h"
#include "ButtonReader.h"
#include "HomeScreen.h"

namespace {
enum class Screen { Home, BadUsb };

TinyScreen display(TinyScreenPlus);
ButtonReader buttons(display);
HomeScreen home(display);
BadUsbScreen badUsb(display);
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
        if (home.openBadUsbOnPress(pressed)) {
            screen = Screen::BadUsb;
            badUsb.enter();
        }
    } else if (badUsb.update(pressed, now)) {
        screen = Screen::Home;
        home.enter();
    }
}
