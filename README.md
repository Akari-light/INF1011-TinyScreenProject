# TraceDeck

TraceDeck is an ICT1011 proof of concept built with a TinyScreen+ and a Windows application called **TraceHub**. It demonstrates two everyday security risks: plugging in an unfamiliar USB device and leaving a workstation unlocked.

- **USB keyboard emulation.** The handheld acts as a keyboard to run Drop & Upload, De-Shield, Pwning (a reverse shell), and Edge-Jar / Chrm-Jar (cookie-key recovery). TraceHub provides independent evidence of each result.
- **Bluetooth LE.** The handheld displays nearby advertisements. A second board can act as a desk beacon for the walk-away audit, which compares a doorway button press with a Windows session-lock event.
- **TraceHub.** The Flask and SQLite application provides a searchable Loot Log, text and image previews from ZIP files, original-file downloads, and a view of saved walk-away trials.

**Educational use.** Use only authorized equipment, synthetic data, and an isolated lab network. Do not use this project for unauthorized access or malicious activity.

## Hardware and software

| Component | Setup |
| --- | --- |
| USB node | TinyScreen+ ASM2022, Micro-USB data cable, and a charged battery for disconnected operation |
| BLE observation | ASD2116-R ST BLE TinyShield on the same TinyScreen+ and an owned BLE advertiser for address-level inspection |
| TraceHub server | Windows with Python 3.11 to run the upload listener and Ops Console |
| Arduino toolchain | Arduino CLI 1.5.1, Arduino SAMD 1.8.14, TinyCircuits SAMD 1.1.0, TinyScreen 1.1.0, Arduino Keyboard 1.0.7, and TinyCircuits STBLE 0.1.0 |

## Set up the TraceHub server

On the server, open PowerShell in the repository root and install the dependencies once. Run the commands from the root so Python can import the `tracehub` package. Do not run `tracehub/__init__.py` directly. Flask serves the web application; the VS Code Live Server extension cannot run its Python routes or database queries.

```powershell
$traceDeckEnvironment = Join-Path $env:LOCALAPPDATA 'TraceDeck\venv'
py -3.11 -m venv $traceDeckEnvironment
$traceDeckPython = Join-Path $traceDeckEnvironment 'Scripts\python.exe'
& $traceDeckPython -m pip install -r .\tracehub\requirements.txt
```

### Ops Console

```powershell
$traceDeckPython = Join-Path $env:LOCALAPPDATA 'TraceDeck\venv\Scripts\python.exe'
$env:TRACEDECK_SHELL_HOST = '127.0.0.1'
& $traceDeckPython -m flask --app tracehub --no-debug run --host 127.0.0.1
```

On the server, open `http://127.0.0.1:5000`. The workspace navigation opens the Loot Log, Walk-away audit, or Reverse Shell console. The overview summarizes captures, source IPs, identified hosts, and known upload sizes. Search, filter by hostname status, or order captures newest or oldest first, then open a row for the full file text and metadata. The log shows up to 50 matches and refreshes every two seconds.

The dashboard also starts a separate unauthenticated TCP listener. Its default bind is all interfaces, independent of Flask's HTTP address. The command above confines it to the same PC. Keep both services local. The `walk` role below starts the walk-away web view without that TCP listener.

### Verify storage with a manual upload

In another PowerShell window at the repository root, start the local upload page. This check uses dummy files and needs no board.

```powershell
$traceDeckPython = Join-Path $env:LOCALAPPDATA 'TraceDeck\venv\Scripts\python.exe'
& $traceDeckPython -m flask --app "tracehub:create_app('badusb_demo')" --no-debug run --host 127.0.0.1 --port 8080
```

1. Open [the local upload page](http://127.0.0.1:8080/badusb/upload). Selecting a synthetic text file or ZIP submits it automatically.
2. Match the acknowledgement's loot ID to a row in the Ops Console. Expand it to see the UTC receipt time, observed IP, and text preview. Browser uploads show **Unknown** for the hostname.
3. Select **Download original files** to check the original. JPG, PNG, GIF, and WebP entries inside a ZIP can appear inline. Open PDFs and other documents from the downloaded original.

The request limit is 8 MiB including form overhead. ZIPs are limited to 100 entries and 16 MiB of expanded data. An identical retry with the same submission ID reuses the loot ID. A conflicting submission is rejected before writing a new archive. The separate authenticated HTTPS text receiver records a client-reported hostname for enrolled helpers.

The app roles share `%LOCALAPPDATA%\TraceDeck\tracehub.sqlite3`. The same directory holds originals in `archives/` and walk journals in `walk-trials/`. Stop the listeners and capture processes before backing up those items together. Keep runtime data, credentials, and certificates out of Git.

## BadUSB payloads

Open **Home → BadUSB → Payloads** on the handheld. A payload whose `showDisabledPayload` guard is still active shows as disabled and does nothing; remove that guard in the payload's `.cpp` before running it.

| Payload | What it does | Network target | Build flag |
| --- | --- | --- | --- |
| Drop & Upload | Writes a marker file, then zips `Documents\Test_Important` and uploads it | `/badusb/upload` on `:8080` | `TRACEHUB_SERVER_IP` |
| De-Shield | Sets UAC to "never notify", then re-enables Virus & Threat Protection | none | none |
| Pwning | Opens a reverse shell back to the operator | raw TCP `:4444` | `ATTACKER_IP` |
| Edge-Jar | Recovers Edge's DPAPI AES key and lists still-encrypted cookies | `/badusb/upload` on `:8080` | `TRACEHUB_SERVER_IP` |
| Chrm-Jar | Same as Edge-Jar, for Chrome | `/badusb/upload` on `:8080` | `TRACEHUB_SERVER_IP` |
| Payload | Placeholder entry | none | none |

Compile the network-dependent payloads with the server address:

```powershell
arduino-cli compile --fqbn TinyCircuits:samd:tinyscreen firmware/TraceDeck `
  --build-property "compiler.cpp.extra_flags=-DTRACEHUB_SERVER_IP=<server> -DATTACKER_IP=<server>"
```

Drop & Upload and the cookie jars read `TRACEHUB_SERVER_IP`; Pwning reads `ATTACKER_IP`.

### Reverse shell (Pwning)

Start TraceHub with the shell listener reachable from the victim laptop, then flash Pwning with `-DATTACKER_IP=<server>`:

```powershell
$env:TRACEDECK_SHELL_HOST = '0.0.0.0'   # or the lab subnet IP
& $python -m flask --app "tracehub:create_app('demo')" --no-debug run --host 0.0.0.0 --port 8080
```

The `demo` role runs the Ops Console, the upload endpoint, the reverse-shell console, and the walk-away view in one process. The reverse shell itself listens on port `4444`, separate from the HTTP port. Open the **Reverse Shell** workspace to watch the callback and send commands.

### Cookie jars (Edge-Jar / Chrm-Jar)

The jars run a "half-step" demonstration: they recover the browser's DPAPI-protected AES master key and enumerate the cookie database **without decrypting** any cookie, so no plaintext session token leaves the machine. The report uploads as `edge_cookie_dump.txt` or `chrome_cookies_dump.txt`.

The target machine needs internet access (for `Install-Module PSSQLite`, which also installs the NuGet provider), UAC at the default "notify" level, and the browser installed with an existing cookie database. The payload force-closes the browser, opens an elevated PowerShell window, uploads the report, then closes the window. Undo the settings it changes before leaving the lab (see Clean up after the demo).

## Prepare the handheld for Bluetooth

With Arduino CLI installed, set up the pinned board packages and libraries.

```powershell
arduino-cli config add board_manager.additional_urls https://files.tinycircuits.com/ArduinoBoards/package_tinycircuits_index.json
arduino-cli core update-index
arduino-cli core install "arduino:samd@1.8.14" "TinyCircuits:samd@1.1.0"
arduino-cli lib update-index
arduino-cli lib install "TinyScreen@1.1.0" "Keyboard@1.0.7"
```

Install the [TinyCircuits STBLE 0.1.0 ZIP](https://github.com/TinyCircuits/TinyCircuits-TinyShield-BLE-ASD2116/tree/2dcc1d42c2e63ca9d489355665cf2769600d94d8/examples) in the Arduino sketchbook. The TinyScreen+ board ID is `TinyCircuits:samd:tinyscreen` and the sketch entry point is `firmware/TraceDeck/TraceDeck.ino`. Use `arduino-cli board list` to identify the connected board's COM port.

Use a reviewed Bluetooth lab image for Bluetooth trials. With the display upright, R1/R2 scroll, L1 opens the selection, and L2 goes back. Use a data cable for USB serial and check the handheld's battery before an untethered trial.

For the USB demonstration, the OLED result describes what the handheld observed during its own sequence. The matching TraceHub record confirms that the server saved the test upload.

## Run BLE Nearby

Fit the ASD2116-R shield with the board powered off. Open **Home → Bluetooth → Nearby scan**. R1 and R2 move through the live list, L1 opens the selected entry, and L2 steps back. Scanning continues while browsing. Returning to the Bluetooth menu stops the scan.

- `LIVE n/10s` counts observed address/type pairs in the preceding 10 seconds. `+` indicates the count is a lower bound because the in-memory cache is full. The same physical device can contribute multiple addresses. List entries can outlast the count and show `stale`.
- Advertised names appear first. All recent addresses without a received name share one `Unnamed N` row. A named entry opens its last received signal in dBm and how recently it was seen. The unnamed group shows its count, the strongest last received signal and the age of the latest observation.
- For an owned test advertiser, use the exact advertised name `TD-PHONE` or `TD-B01`. Alternatively use test manufacturer ID `0xFFFF` with the ASCII marker as data. The matching row can be selected for address-level serial diagnostics. The optional suffix `;OS=<platform>` labels an explicitly self-reported platform in serial.
- At 115200 baud with DTR enabled, USB serial emits scan health and count data. Only a selected lab-marked entry emits its address and raw packets over serial. For a lock/unlock experiment, hold the phone's location and advertiser configuration constant, mark each phase, and compare packets only if the phone's observation can be identified. Record missing or ambiguous identification as inconclusive.

The scanner uses the BlueNRG-MS observation procedure in TinyCircuits STBLE, with HCI scan commands as a fallback. Manufacturer IDs are decoded from advertising data. [ST BlueNRG-MS ACI manual](https://www.st.com/resource/en/user_manual/um1865-the-bluenrgms-bluetooth-le-stack-application-command-interface-aci-stmicroelectronics.pdf) · [Bluetooth SIG Assigned Numbers](https://www.bluetooth.com/wp-content/uploads/Files/Specification/HTML/Assigned_Numbers/out/en/index-en.html)

## Walk-away lock audit

Use this test on a team's Windows workstation to measure the interval between losing sight of the keyboard and Windows reporting a session lock. The desk beacon is a radio reference. Windows independently monitors its paired phone for Dynamic Lock. [Microsoft Dynamic Lock](https://learn.microsoft.com/en-us/windows/security/identity-protection/hello-for-business/hello-feature-dynamic-lock) · [Windows session notifications](https://learn.microsoft.com/en-us/windows/win32/termserv/wm-wtssession-change)

1. Fit the ASD2116-R shield to each powered-off TinyScreen+. Charge the handheld battery and verify that disconnecting its USB cable keeps the display on. Identify each board and keep a verified restore image before flashing. Use the same TraceDeck sketch on both boards. On the stationary board, open **Home → Bluetooth → Desk beacon** to advertise `TD-B01`. L1 pauses or resumes advertising. L2 stops it and returns to the Bluetooth menu. On the handheld, choose **Home → Bluetooth → Walk-away**, or open the `TD-B01` details in **Nearby scan** and press L1.
2. Pair the owned phone to the assessment PC and check **Settings → Accounts → Sign-in options → Dynamic lock**. Record enabled/disabled status and other auto-lock or sleep timers. Start with the physical console session unlocked.
3. In the repository root, update the Windows Python environment and start the local trial view. Open `http://127.0.0.1:5074/walk-away`. The regular Ops Console also offers the Walk-away audit workspace.

   ```powershell
   $python = Join-Path $env:LOCALAPPDATA 'TraceDeck/venv/Scripts/python.exe'
   & $python -m pip install -r ./tracehub/requirements.txt
   & $python -m flask --app "tracehub:create_app('walk')" --no-debug run --host 127.0.0.1 --port 5074
   ```

4. In a second PowerShell window set the same `$python` variable. Run the manual control, press Win+L, then sign in. Refresh the trial view and verify **Manual control observed**. Use a fresh trial for the actual walk.

   ```powershell
   & $python -m tracehub.walk_capture control
   $walkPort = Read-Host 'Walk-away handheld application COM port'
   & $python -m tracehub.walk_capture walk --port $walkPort --route "Service desk to doorway" --configuration "Record actual phone pairing, Dynamic Lock, sleep and idle settings"
   ```

5. Use the handheld's identified application port. Arming requires a desk-beacon observation less than ten seconds old. When the OLED shows **Ready to mark**, leave the station idle and carry the paired phone and the battery-powered handheld. At the first point where the keyboard is out of sight, press **L1** once. Confirm **Doorway marked**. Remain there for at least 90 seconds to measure the full window. Return, reconnect the handheld and allow the capture to finish. On **Trial saved**, refresh TraceHub. A marked trial remains in RAM until the PC acknowledges the save. To abandon one deliberately, press **L2**, then **L1** to confirm or **L2** to keep it.
6. TraceHub stores the button time, latest desk-beacon signal and age at the button press, Windows lock time, result and timing uncertainty. It uses the same assessment PC for both clock exchanges and the local WTS event observer. Doorway time is mapped from the board tick to the PC clock. Windows times are local receipt times for session notifications. Displayed uncertainty covers clock alignment. Windows scheduling can add notification delay. Keep the setting and the observed outcome together when interpreting a run.

Results include **Lock observed**, **Already locked at marker**, **No lock observed within 90 s**, or **Inconclusive** with a reason. A negative finding requires continuous observer coverage. A stale radio reading is valid at the doorway. A missing observer, session change, sleep, board reset, or ambiguous clock order can make the result inconclusive. Completed journals are in `%LOCALAPPDATA%\TraceDeck\walk-trials\<trial-id>.json`. Re-import a finalized journal with `python -m tracehub.walk_capture import "<file>"`. Identical imports keep one database entry. Refresh the console to see the result after restart.

For the assessment, film the marked doorway, the physical OLED confirmation and the matching trial ID in TraceHub. Repeat the same route with the paired phone left on the desk as a comparison. Show the measured values and identify the source of each timestamp.

## Clean up after the demo

Unplug the handheld and stop local services with Ctrl+C. Restore the Windows settings and phone pairing changed for the demo. Review demo-created Desktop files, temporary archives, and any firewall exceptions added for the lab. Preserve original sample files and any evidence needed for assessment. Restore any overwritten files from a pre-demo backup.

After running Edge-Jar or Chrm-Jar, undo the settings they change on the target machine:

```powershell
Set-ExecutionPolicy -Scope LocalMachine -ExecutionPolicy <original>
Uninstall-Module -Name PSSQLite -Force -ErrorAction SilentlyContinue
Remove-Item -Recurse -Force "$env:ProgramFiles\PackageManagement\ProviderAssemblies\NuGet" -ErrorAction SilentlyContinue
Remove-Item -Recurse -Force "$env:LOCALAPPDATA\PackageManagement\ProviderAssemblies\NuGet" -ErrorAction SilentlyContinue
Remove-Item -Force "$env:TEMP\edge_cookie_dump.txt","$env:TEMP\chrome_cookies_dump.txt" -ErrorAction SilentlyContinue
```

## Verification

Run the existing Windows tests from the repository root.

```powershell
$traceDeckPython = Join-Path $env:LOCALAPPDATA 'TraceDeck\venv\Scripts\python.exe'
& $traceDeckPython -B -m unittest discover -s tests -v
```

The suite covers walk-audit timing, incomplete observations, and evidence storage. For a physical rehearsal, record the revision and observed results separately. Match a synthetic upload to its saved record, verify BLE observations with an owned beacon, and confirm that the walk-away view shows the Windows lock evidence. Check that saved evidence remains visible after restarting TraceHub.

## Repository layout

- `firmware/TraceDeck/` contains the Arduino sketch, shared UI and input code, the BadUSB payloads under `src/badusb/payloads/` (Drop & Upload, De-Shield, Pwning, Edge-Jar, Chrm-Jar, and a Payload stub), and the scanner, walk-away audit and desk beacon under `src/bluetooth/`.
- `scripts/` holds reference copies of the payload sources and the original Duckyscript research, kept outside the compiled sketch.
- `tracehub/__init__.py` holds the Flask application factory. `dashboard/` contains the Ops Console routes, and `walk/` contains the walk-away routes, trial evaluation, and Windows capture. The documented `tracehub.walk_capture` command calls the capture module. Shared SQLite code is in `database.py`, HTML templates are under `templates/`, and shared console assets are under `static/`.
- `badusb_upload.py` receives browser file uploads, `loot_receiver.py` handles the separate credentialed HTTPS text receiver, and `reverse_shell.py` owns the raw TCP sessions and local shell console routes. These modules are under `tracehub/`.
- `tests/` contains the existing automated walk-audit checks.

## ICT1011 delivery checklist

Verify the source and documentation ZIP from an extracted copy. Check that the 2–3 minute video shows the application, the demonstration, the team's division of work, and source credits. Attribute reused materials in the source and video. Confirm the course submission and live demo arrangements, and rehearse the behavior shown in the video.
