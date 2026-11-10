#include "DropUpload.h"

#include <Keyboard.h>

#define TRACEDECK_STRINGIFY_LITERAL(value) #value
#define TRACEDECK_STRINGIFY(value) TRACEDECK_STRINGIFY_LITERAL(value)

namespace {
constexpr uint32_t kDialogWaitMs = 1750;
constexpr uint32_t kNotepadLaunchWaitMs = 2500;
constexpr uint32_t kNotepadSaveWaitMs = 2500;
constexpr uint32_t kBrowserLaunchWaitMs = 2500;

#ifdef TRACEHUB_PC1_IP
constexpr char kTraceHubUploadUrl[] =
    "http://" TRACEDECK_STRINGIFY(TRACEHUB_PC1_IP) ":8080/badusb/upload";
#else
constexpr char kTraceHubUploadUrl[] = "";
#endif

void pressShortcut(uint8_t modifier, uint8_t key) {
    Keyboard.press(modifier);
    Keyboard.press(key);
    Keyboard.releaseAll();
}

void openFromWindowsRun(const char *target) {
    pressShortcut(KEY_LEFT_GUI, 'r');
    delay(kDialogWaitMs);
    pressShortcut(KEY_LEFT_CTRL, 'a');
    Keyboard.print(target);
    Keyboard.write(KEY_RETURN);
}

void enterDemoFilePath() {
    pressShortcut(KEY_LEFT_ALT, 'd');
    Keyboard.print("%TEMP%");
    Keyboard.write(KEY_RETURN);
    delay(kDialogWaitMs);

    pressShortcut(KEY_LEFT_ALT, 'n');
    pressShortcut(KEY_LEFT_CTRL, 'a');
    Keyboard.print("hello-world.txt");
    Keyboard.write(KEY_RETURN);
}

void sendNotepadSaveKeys() {
    openFromWindowsRun("notepad.exe");
    delay(kNotepadLaunchWaitMs);

    // Start a new document in case Notepad restored an earlier tab.
    pressShortcut(KEY_LEFT_CTRL, 'n');
    delay(kDialogWaitMs);
    Keyboard.print("hello world");
    pressShortcut(KEY_LEFT_CTRL, 's');
    delay(kDialogWaitMs);

    enterDemoFilePath();
}
}

void sendBadUsbDemoKeys() {
    Keyboard.begin(KeyboardLayout_en_US);
    sendNotepadSaveKeys();

    if (kTraceHubUploadUrl[0] == '\0') {
        return;
    }

    // Let Save As finish before Windows Run takes focus again.
    delay(kNotepadSaveWaitMs);
    if (!USBDevice.configured()) {
        return;
    }

    openFromWindowsRun(kTraceHubUploadUrl);
    delay(kBrowserLaunchWaitMs);
    Keyboard.write(KEY_RETURN);
    delay(kDialogWaitMs);
    enterDemoFilePath();
}
