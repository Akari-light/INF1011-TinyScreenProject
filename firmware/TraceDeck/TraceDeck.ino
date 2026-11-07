#include <TinyScreen.h>

#include "UsbTestScreen.h"

namespace {
TinyScreen display(TinyScreenPlus);
UsbTestScreen usbTest(display);
}

void setup() {
    display.begin();
    display.setBrightness(10);
    usbTest.enter();
}

void loop() {
    usbTest.update();
}
