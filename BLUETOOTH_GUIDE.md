# TraceDeck Bluetooth guide

Use **Nearby scan** to see what the radio can receive, **Desk beacon** to broadcast a known signal, and **Walk-away** to measure how long a Windows workstation stays unlocked after its operator leaves sight of the keyboard. Start with Nearby scan. It works with one TinyScreen+ and gives an immediate result on the OLED.

## 1. Know which device does what

- **Handheld TinyScreen+.** Fit the ASD2116-R ST BLE TinyShield to the ASM2022 with power off. Leave the Wi-Fi shield detached. Use USB while scanning at the desk. For a walk, attach a charged battery, switch the handheld on, and check that its display stays on when unplugged. Do this battery check **before** starting a trial.
- **Desk TinyScreen+.** For the two-board test, a second TinyScreen+ with the same BLE shield stays powered at the workstation. It advertises the team marker `TD-B01`. Both boards use the same Bluetooth menu. Choose their roles on the OLED.
- **Windows laptop and phone.** Windows uses its paired phone for Dynamic Lock. The handheld listens to the desk beacon separately. The tester carries both the phone and the handheld. The phone can participate even when its name never appears in Nearby scan. [Microsoft Dynamic Lock](https://learn.microsoft.com/en-us/windows/security/identity-protection/hello-for-business/hello-feature-dynamic-lock)

Hold the board with the OLED upright. **L1** is top-left, L2 bottom-left, R1 top-right, R2 bottom-right. In a menu, R2 moves down, R1 moves up, L1 opens the highlighted row and L2 goes back. From a fresh Home screen, press **R2** to highlight **BLUETOOTH**, then **L1** to open it. The three rows are **Walk-away**, **Desk beacon**, and **Nearby scan**.

## 2. Bring the workspace to the laptop

The board that already shows all three menu entries can be used immediately. Moving it to a new laptop leaves its firmware on the board. Arduino CLI is useful for identifying the port and for future firmware work. The Windows observer needs Python.

1. Sync the current project folder. In PowerShell, change to the project root, where `README.md` and the `tracehub` folder are visible. These commands should both return `True`.

   ```powershell
   Test-Path .\firmware\TraceDeck\src\bluetooth\BluetoothMenu.cpp
   Test-Path .\tracehub\walk_capture.py
   ```

   At the 24 November checkpoint, the integrated Bluetooth folder and walk-capture files were still local to the working copy. The published scanner branch alone is insufficient for this walkthrough.

2. For firmware work, use Arduino CLI **1.5.1**, board target `TinyCircuits:samd:tinyscreen`, cores **arduino:samd 1.8.14** and **TinyCircuits:samd 1.1.0**, and libraries **TinyScreen 1.1.0**, **Keyboard 1.0.7**, and **TinyCircuits STBLE 0.1.0**. The STBLE ZIP supplies `STBLE.h`. [Project installation commands](README.md#build-the-handheld)
3. Connect only the handheld you are identifying. Run `arduino-cli board list` and record its **application** COM port. The original desktop used COM8. Use the value Windows assigns on the laptop. Keep a second board that is in use for another project in its current role.
4. For the Windows audit, install Python 3.11 and create an environment on the new laptop. Run these commands from the project root.

   ```powershell
   $venv = Join-Path $env:LOCALAPPDATA 'TraceDeck\venv'
   py -3.11 -m venv $venv
   $python = Join-Path $venv 'Scripts\python.exe'
   & $python -m pip install -r .\tracehub\requirements.txt
   ```

## 3. Try Nearby scan with one board

1. In **Bluetooth**, use **R2** until **Nearby scan** is highlighted. Press **L1**. Allow several seconds for reports.
2. Read **LIVE n/10s**. This is the count of distinct advertising address/type pairs heard in the last ten seconds. An advertiser can rotate addresses. A trailing `+` means the cache filled and the count is a lower bound.
3. Names appear first. Use **R1/R2** to highlight a name, then **L1** for details. **Signal** is the last received RSSI in dBm. For example, −55 dBm is stronger than −80 dBm. **Last seen** is the age of the latest report. Scanning continues on the detail screen.
4. Open **Unnamed N** to see the group count, strongest recent signal and most recent observation. A phone or laptop with Bluetooth enabled may advertise without a usable local name. A manufacturer field can identify a company. The device model, OS and phone lock state can still be unknown.
5. Press **L2** to return to the list, then **L2** again for the Bluetooth menu. This stops the scan.

For a USB evidence check, use a serial monitor at **115200 baud** with DTR enabled. The scanner emits `BLE|` health and count records. Selecting an owned `TD-B01` or `TD-PHONE` marker also produces `BLE-LAB|` with address, reported address type, RSSI, advertisement event and bytes, and a separate scan-response event and bytes when received. An explicit platform label is self-reported. Close the serial monitor before running the walk collector so it can open the port.

## 4. Check the two radios with a known beacon

Do this when the second TinyScreen+ is available and already has the Bluetooth menu. It can use battery or USB power at the desk.

If the spare still needs firmware, the team’s verified Bluetooth-menu test image is 48,828 bytes with SHA-256 `936DF558B1891A87BE00F3C002F88AA222856343E9A2042EC6EF4D5452850E08`. It contains the integrated Bluetooth menu with USB payloads inactive. Preserve a restorable copy of the spare's existing firmware before flashing. With only the intended spare connected, the following optional commands upload that specific image.

```powershell
$image = Read-Host 'Full path to the verified Bluetooth-menu .bin'
if ((Get-FileHash -LiteralPath $image).Hash -ne '936DF558B1891A87BE00F3C002F88AA222856343E9A2042EC6EF4D5452850E08') {
    throw 'This file does not match the Bluetooth-menu test image.'
}
arduino-cli board list
$sparePort = Read-Host 'Application COM port of the spare board'
arduino-cli upload --fqbn TinyCircuits:samd:tinyscreen --port $sparePort --input-file $image .\firmware\TraceDeck
```

1. **Desk board:** choose **Bluetooth → Desk beacon** and press **L1**. Look for **TD-B01** and **Advertising**.
2. **Handheld:** choose **Bluetooth → Nearby scan**. Highlight **TD-B01** and press **L1**. Look for a signal reading and a fresh **Last seen**.
3. **Desk board:** press **L1** once. Its screen should say **Paused**. Watch the handheld for at least ten seconds. The selected entry should age to **Last seen 10s+** while other advertisements can keep arriving.
4. **Desk board:** press **L1** again. Check that **Advertising** returns and the handheld shows a fresh observation. Leave the desk board in this mode for the walk. **L2** on the desk board exits the screen and stops the transmitter.

This check proves reception, loss and recovery of a known signal. It also isolates radio problems before the Windows test. The beacon advertises its local name and service UUID in the advertisement and supplies test manufacturer data in its scan response. [ST BlueNRG-MS ACI manual](https://www.st.com/resource/en/user_manual/um1865-the-bluenrgms-bluetooth-le-stack-application-command-interface-aci-stmicroelectronics.pdf)

## 5. Check that the laptop records a real lock

On the laptop being assessed, pair your phone to Windows. Open **Settings → Accounts → Sign-in options → Dynamic lock** and record its actual setting. Also record idle-lock and sleep settings that might affect the result. Windows must be idle for Dynamic Lock to act. [Microsoft Dynamic Lock](https://learn.microsoft.com/en-us/windows/security/identity-protection/hello-for-business/hello-feature-dynamic-lock)

**PowerShell window A — trial dashboard.** From the project root, run the following and leave the window open. Open `http://127.0.0.1:5074/walk-away` on the same laptop.

```powershell
$python = Join-Path $env:LOCALAPPDATA 'TraceDeck\venv\Scripts\python.exe'
& $python -m flask --app "tracehub:create_app('walk')" run --host 127.0.0.1 --port 5074
```

**PowerShell window B — manual control.** Open a second window in the project root. Start with the local Windows session unlocked.

```powershell
$python = Join-Path $env:LOCALAPPDATA 'TraceDeck\venv\Scripts\python.exe'
& $python -m tracehub.walk_capture control
```

When prompted, press **Win+L**, wait a few seconds, then sign back in. Refresh the dashboard. Expect a new **Manual control observed** record. This checks the session observer on this laptop. The physical walk gets its own trial ID. [Windows session notifications](https://learn.microsoft.com/en-us/windows/win32/termserv/wm-wtssession-change)

## 6. Run the walk-away audit

1. Leave the desk beacon **Advertising**. Plug the battery-powered handheld into the laptop and choose **Bluetooth → Walk-away**. Expect **Dock to arm**. With the beacon nearby, **Beacon 0s ago** means the handheld heard it within the last second.
2. In PowerShell window B, run the commands below. Enter the handheld's laptop COM port and the actual settings when prompted. Leave this process running through undocking and redocking.

   ```powershell
   $port = Read-Host 'Handheld application port, for example COM8'
   $settings = Read-Host 'Phone pairing, Dynamic Lock, idle lock and sleep settings'
   & $python -m tracehub.walk_capture walk --port $port --route "Service desk to doorway" --configuration $settings
   ```

3. Wait for **Ready to mark**. Arming requires a desk-beacon sighting less than ten seconds old. The PC and handheld now share a trial ID and an initial clock measurement.
4. Unplug the **handheld's USB cable** with the handheld switched on. Leave the laptop untouched. Carry your paired phone and the handheld along the agreed route. **Press L1 exactly once when you first lose sight of the keyboard.** Look for **Doorway marked**. Pressing L1 at the desk would timestamp the desk instead.
5. Stay at the marked location for at least **90 seconds**. Return and reconnect the handheld to the same laptop USB port. Keep the collector running until the OLED says **Trial saved** or the terminal reports the outcome. Refresh the dashboard and match the trial ID printed in window B.

The handheld keeps the mark in RAM and reports the desk beacon's age and RSSI at the press. Windows independently records session events. The collector compares clocks again at redock and calculates **Windows lock time minus doorway press time**. If you need to abandon an armed trial, **L2** opens **Discard trial?**, **L1** confirms, and **L2** keeps the trial. Turning the board off loses an unsaved mark.

## 7. Read the result and keep the evidence

- **Lock observed** means Windows reported a lock after the doorway press. Report the elapsed seconds, clock uncertainty, route and configured Windows controls. A Windows lock event alone cannot identify which configured control caused it.
- **Already locked at marker** means the observed session had locked before the button press.
- **No lock observed within 90 s** means the observer covered the entire test window without a lock in that interval. The window is measured from the button press. Windows makes its own decision about when the paired phone has moved out of range.
- **Inconclusive** includes a missing mark, session or observer interruption, or clock disagreement. Record the reason from TraceHub. Losing sight of `TD-B01` is radio evidence only.

For assessment, keep the trial ID, route and settings, a photo of the handheld confirmation, and the matching dashboard result. The per-trial JSON is under `%LOCALAPPDATA%\TraceDeck\walk-trials\` on the **capture PC**. These files live outside the project folder. To bring a completed trial to the laptop, copy its JSON and run the following from window B. Refresh the dashboard afterward. An identical re-import keeps one record.

```powershell
$journal = Read-Host 'Full path to the completed trial JSON'
& $python -m tracehub.walk_capture import $journal
```

## If a step stalls

- **`STBLE.h` missing during a build.** Check `arduino-cli lib list` for **STBLE 0.1.0**. Install the pinned ZIP into the sketchbook reported by `arduino-cli config get directories.user`.
- **`Finding TD-B01`.** Check the desk OLED for **Advertising**. Bring the boards together and try Nearby scan. If the radio reports an error, power the board off before reseating the ASD2116-R shield.
- **`Dock to arm` plus a fresh beacon.** Check that window B is running the **walk** command, that it is using the handheld's current port, and that the serial monitor is closed. The dashboard alone cannot arm a trial.
- **Screen goes dark on unplug.** Check the battery and power switch before starting a new trial. A restarted board loses its old trial ID.
- **No lock appeared.** Check the manual Win+L control first. If it passes, examine phone pairing, Dynamic Lock, the route and other timers. A keyboard out of sight can still be within Bluetooth range.

**Evidence as of 24 November 2026.** The operator saw the three menu entries on COM8. Earlier physical runs received the owned beacon, aged it out, and saw fresh reports resume. The operator also read **Ready to mark** and **Beacon 0s ago** during the PC-beacon bench check. On the original PC, Windows recorded a manual lock and unlock and TraceHub displayed that control. The integrated menu and transmitter controls passed emulation, and nine walk-audit regression tests passed. The paused walk had no recorded doorway press and finished **Inconclusive**. A measured walk-away interval and a two-board rehearsal with the final menu build remain pending.
