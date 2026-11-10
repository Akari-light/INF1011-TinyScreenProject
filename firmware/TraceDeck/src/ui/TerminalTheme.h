#pragma once

#include <TinyScreen.h>

namespace TerminalTheme {

// TinyScreen's native 16-bit format puts blue in the high bits and red in the low bits.
constexpr uint16_t color(uint8_t red, uint8_t green, uint8_t blue) {
    return ((blue >> 3) << 11) | ((green >> 2) << 5) | (red >> 3);
}

constexpr uint16_t kBackground = TS_16b_Black;
constexpr uint16_t kFrame = color(18, 109, 127);
constexpr uint16_t kCyan = color(50, 228, 246);
constexpr uint16_t kGreen = TS_16b_Green;
constexpr uint16_t kAmber = color(255, 187, 70);
// Compensate for the panel's purple cast at neutral midtones.
constexpr uint16_t kMuted = color(130, 170, 118);
constexpr uint16_t kControl = color(179, 183, 255);

void drawFrame(TinyScreen &display);
void drawText(TinyScreen &display, uint8_t x, uint8_t y, uint16_t foreground, const char *text,
              const FONT_INFO &font = thinPixel7_10ptFontInfo);

}
