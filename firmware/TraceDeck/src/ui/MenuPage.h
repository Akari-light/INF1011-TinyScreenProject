#pragma once

#include <TinyScreen.h>

class MenuPage {
public:
    using LabelAt = const char *(*)(size_t index);
    MenuPage(TinyScreen &display, LabelAt labelAt, size_t count);

    void enter();
    bool move(uint8_t pressed);
    size_t selected() const { return selected_; }

private:
    void drawRows();

    TinyScreen &display_;
    LabelAt labelAt_;
    size_t count_;
    size_t selected_ = 0;
    size_t firstVisible_ = 0;
};
