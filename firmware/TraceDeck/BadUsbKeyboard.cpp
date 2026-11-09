#include "BadUsbKeyboard.h"

#include <Keyboard.h>

namespace {
constexpr uint32_t kDialogWaitMs = 750;
constexpr uint32_t kNotepadLaunchWaitMs = 2000;

void pressShortcut(uint8_t modifier, uint8_t key) {
    Keyboard.press(modifier);
    Keyboard.press(key);
    Keyboard.releaseAll();
}
}

void sendNotepadSaveKeys() {
    Keyboard.begin(KeyboardLayout_en_US);

    pressShortcut(KEY_LEFT_GUI, 'r');
    delay(kDialogWaitMs);
    pressShortcut(KEY_LEFT_CTRL, 'a');
    Keyboard.print("notepad.exe");
    Keyboard.write(KEY_RETURN);
    delay(kNotepadLaunchWaitMs);

    // Start a new document in case Notepad restored an earlier tab.
    pressShortcut(KEY_LEFT_CTRL, 'n');
    delay(kDialogWaitMs);
    Keyboard.print("hello world");
    pressShortcut(KEY_LEFT_CTRL, 's');
    delay(kDialogWaitMs);

    pressShortcut(KEY_LEFT_ALT, 'd');
    Keyboard.print("%TEMP%");
    Keyboard.write(KEY_RETURN);
    delay(kDialogWaitMs);

    pressShortcut(KEY_LEFT_ALT, 'n');
    pressShortcut(KEY_LEFT_CTRL, 'a');
    Keyboard.print("hello-world.txt");
    Keyboard.write(KEY_RETURN);
}
