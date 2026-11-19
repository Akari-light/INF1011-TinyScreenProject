#include "MenuPage.h"

#include "TerminalTheme.h"

namespace {
constexpr uint8_t kTop = 16;
constexpr uint8_t kRowHeight = 11;
constexpr uint8_t kRows = 3;
constexpr uint8_t kHeight = kRows * kRowHeight;
constexpr uint8_t kTextX = 15;
constexpr uint8_t kLabelWidth = 75;
}

MenuPage::MenuPage(TinyScreen &display, LabelAt labelAt, size_t count)
    : display_(display), labelAt_(labelAt), count_(count) {}

void MenuPage::enter() {
    using namespace TerminalTheme;
    drawFrame(display_);
    display_.drawLine(27, 3, 27, 10, kFrame);
    display_.drawLine(85, 7, 89, 3, kControl);
    display_.drawLine(89, 3, 93, 7, kControl);
    display_.drawLine(89, 3, 89, 11, kControl);
    display_.drawLine(85, 57, 89, 61, kControl);
    display_.drawLine(89, 61, 93, 57, kControl);
    display_.drawLine(89, 53, 89, 61, kControl);
    drawRows();
}

bool MenuPage::move(uint8_t pressed) {
    if (count_ < 2) {
        return false;
    }
    if (pressed == TSButtonUpperRight) {
        selected_ = selected_ == 0 ? count_ - 1 : selected_ - 1;
    } else if (pressed == TSButtonLowerRight) {
        selected_ = selected_ == count_ - 1 ? 0 : selected_ + 1;
    } else {
        return false;
    }

    if (selected_ < firstVisible_) {
        firstVisible_ = selected_;
    } else if (selected_ - firstVisible_ >= kRows) {
        firstVisible_ = selected_ - kRows + 1;
    }
    drawRows();
    return true;
}

void MenuPage::drawRows() {
    using namespace TerminalTheme;
    display_.clearWindow(4, kTop, 90, kHeight);
    display_.setFont(thinPixel7_10ptFontInfo);
    for (uint8_t row = 0; row < kRows && row < count_ - firstVisible_; ++row) {
        const size_t index = firstVisible_ + row;
        const uint8_t y = kTop + row * kRowHeight;
        const bool focused = index == selected_;
        if (focused) {
            display_.drawLine(5, y + 2, 9, y + 5, kCyan);
            display_.drawLine(9, y + 5, 5, y + 8, kCyan);
        }

        // TinyScreen's font width function accepts printable ASCII only.
        char label[32];
        size_t length = 0;
        const char *source = labelAt_(index);
        while (length < sizeof(label) - 1 && source[length] != '\0') {
            const char glyph = source[length];
            label[length++] = glyph >= ' ' && glyph <= '~' ? glyph : '?';
        }
        label[length] = '\0';
        if (source[length] != '\0' || display_.getPrintWidth(label) > kLabelWidth) {
            do {
                label[--length] = '~';
                label[length + 1] = '\0';
            } while (display_.getPrintWidth(label) > kLabelWidth);
        }
        drawText(display_, kTextX, y, focused ? kGreen : kMuted, label);
    }

    if (count_ > kRows) {
        constexpr uint8_t kMaxMarks = 6;
        constexpr uint8_t kLastX = 77;
        constexpr uint8_t kY = 57;
        constexpr uint8_t kSpacing = 4;
        constexpr uint16_t kOffscreenInk = color(91, 114, 85);
        const uint8_t markCount = count_ < kMaxMarks ? count_ : kMaxMarks;
        const uint8_t span = (markCount - 1) * kSpacing;
        const uint8_t firstX = kLastX - span;
        const auto markAt = [&](size_t index) {
            return uint64_t(index) * markCount / count_;
        };
        const uint8_t selectedMark = markAt(selected_);
        const uint8_t firstVisibleMark = markAt(firstVisible_);
        const uint8_t lastVisibleMark = markAt(firstVisible_ + kRows - 1);

        display_.clearWindow(firstX - 1, kY, span + 3, 1);
        for (uint8_t mark = 0; mark < markCount; ++mark) {
            const uint8_t x = firstX + mark * kSpacing;
            if (mark == selectedMark) {
                display_.drawLine(x - 1, kY, x + 1, kY, kAmber);
            } else {
                const bool visible = mark >= firstVisibleMark && mark <= lastVisibleMark;
                display_.drawRect(x, kY, 1, 1, TSRectangleFilled, visible ? kCyan : kOffscreenInk);
            }
        }
    }
}
