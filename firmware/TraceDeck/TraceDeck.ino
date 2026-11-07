#include <TinyScreen.h>

#include "ButtonReader.h"
#include "UsbTestScreen.h"

namespace {
TinyScreen display(TinyScreenPlus);
ButtonReader buttons(display);
UsbTestScreen usbTest(display);
}

void setup() {
    display.begin();
    display.setBrightness(10);
    buttons.begin(millis());
    usbTest.enter();
}

void loop() {
    const uint32_t now = millis();
    const uint8_t pressed = buttons.readPress(now);
    usbTest.update(pressed, now);
}
