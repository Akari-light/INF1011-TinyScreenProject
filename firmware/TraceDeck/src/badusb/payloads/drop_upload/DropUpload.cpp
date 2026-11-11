#include "DropUpload.h"

#include <Keyboard.h>

#define TRACEDECK_STRINGIFY_LITERAL(value) #value
#define TRACEDECK_STRINGIFY(value) TRACEDECK_STRINGIFY_LITERAL(value)

namespace
{
    constexpr uint32_t kDialogWaitMs = 1750;
    constexpr uint32_t kNotepadLaunchWaitMs = 2500;
    constexpr uint32_t kNotepadSaveWaitMs = 2500;
    constexpr uint32_t kBrowserLaunchWaitMs = 2500;
    constexpr uint32_t kNotepadTotalWaitMs = kNotepadLaunchWaitMs + 4 * kDialogWaitMs;
    constexpr uint32_t kUploadTotalWaitMs = kNotepadSaveWaitMs + kBrowserLaunchWaitMs + 3 * kDialogWaitMs;
    constexpr uint32_t kProgressRefreshMs = 125;

#ifdef TRACEHUB_PC1_IP
    constexpr char kTraceHubUploadUrl[] =
        "http://" TRACEDECK_STRINGIFY(TRACEHUB_PC1_IP) ":8080/badusb/upload";
#else
    constexpr char kTraceHubUploadUrl[] = "";
#endif

    struct TimedProgress
    {
        const BadUsbProgress &progress;
        uint32_t totalMs;
        uint32_t completedMs;

        void wait(uint32_t duration)
        {
            const uint32_t startedAt = millis();
            uint32_t elapsed = 0;
            while (elapsed < duration)
            {
                const uint32_t remaining = duration - elapsed;
                delay(remaining < kProgressRefreshMs ? remaining : kProgressRefreshMs);
                elapsed = millis() - startedAt;
                const uint32_t waited = elapsed < duration ? elapsed : duration;
                // Reserve a full bar until the keyboard sequence returns.
                progress.update(progress.context, (completedMs + waited) * 99 / totalMs);
                elapsed = millis() - startedAt;
            }
            completedMs += duration;
        }
    };

    void pressShortcut(uint8_t modifier, uint8_t key)
    {
        Keyboard.press(modifier);
        Keyboard.press(key);
        Keyboard.releaseAll();
    }

    void openFromWindowsRun(const char *target, TimedProgress &timing)
    {
        pressShortcut(KEY_LEFT_GUI, 'r');
        timing.wait(kDialogWaitMs);
        pressShortcut(KEY_LEFT_CTRL, 'a');
        Keyboard.print(target);
        Keyboard.write(KEY_RETURN);
    }

    void enterDemoFilePath(TimedProgress &timing)
    {
        pressShortcut(KEY_LEFT_ALT, 'd');
        Keyboard.print("%TEMP%");
        Keyboard.write(KEY_RETURN);
        timing.wait(kDialogWaitMs);

        pressShortcut(KEY_LEFT_ALT, 'n');
        pressShortcut(KEY_LEFT_CTRL, 'a');
        Keyboard.print("hello-world.txt");
        Keyboard.write(KEY_RETURN);
    }

    void sendNotepadSaveKeys(TimedProgress &timing)
    {
        openFromWindowsRun("notepad.exe", timing);
        timing.wait(kNotepadLaunchWaitMs);

        // Start a new document in case Notepad restored an earlier tab.
        pressShortcut(KEY_LEFT_CTRL, 'n');
        timing.wait(kDialogWaitMs);
        Keyboard.print("hello world");
        pressShortcut(KEY_LEFT_CTRL, 's');
        timing.wait(kDialogWaitMs);

        enterDemoFilePath(timing);
    }
}

void sendBadUsbDemoKeys(const BadUsbProgress &progress)
{
    const bool uploadEnabled = kTraceHubUploadUrl[0] != '\0';
    TimedProgress timing{progress, kNotepadTotalWaitMs + (uploadEnabled ? kUploadTotalWaitMs : 0), 0};
    Keyboard.begin(KeyboardLayout_en_US);
    sendNotepadSaveKeys(timing);

    if (!uploadEnabled)
    {
        return;
    }

    // Let Save As finish before Windows Run takes focus again.
    timing.wait(kNotepadSaveWaitMs);
    if (!USBDevice.configured())
    {
        return;
    }

    openFromWindowsRun(kTraceHubUploadUrl, timing);
    timing.wait(kBrowserLaunchWaitMs);
    Keyboard.write(KEY_RETURN);
    timing.wait(kDialogWaitMs);
    enterDemoFilePath(timing);
}
