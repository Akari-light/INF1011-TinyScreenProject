#include "TerminalTheme.h"

namespace TerminalTheme {

void drawFrame(TinyScreen &display) {
    display.clearScreen();
    display.drawLine(4, 14, 92, 14, kFrame);
    display.drawLine(4, 14, 24, 14, kCyan);
    display.drawLine(4, 50, 92, 50, kFrame);
}

void drawText(TinyScreen &display, uint8_t x, uint8_t y, uint16_t foreground, const char *text,
              const FONT_INFO &font) {
    display.setFont(font);
    display.fontColor(foreground, kBackground);
    display.setCursor(x, y);
    display.print(text);
}

}
