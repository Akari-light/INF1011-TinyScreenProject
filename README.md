# TraceDeck

TraceDeck is an ICT1011 security lab for owned equipment on an isolated network. Home opens the **BadUSB** and **Bluetooth** menus. Drop & Upload saves a Desktop message, closes Notepad, and uses built-in PowerShell to upload a ZIP of the prepared dummy folder. The **TraceHub server** stores the archive and displays its text in the Ops Console. BLE Nearby shows legacy Bluetooth LE advertisements on the TinyScreen+ OLED.

See [PROJECT_PLAN.md](PROJECT_PLAN.md) for the roadmap and test status.

## Hardware and software

| Component | Setup |
| --- | --- |
| USB node | TinyScreen+ ASM2022, Micro-USB data cable, and a charged battery for the USB diagnostic |
| BLE observation | ASD2116-R ST BLE TinyShield on the same TinyScreen+ and an owned BLE advertiser for address-level inspection |
| TraceHub server | Windows with Python 3.11 to run the upload listener and Ops Console |
| Target laptop | Owned, unlocked Windows laptop with Notepad, a default browser, English (US) keyboard layout, and Caps Lock off |
| Arduino toolchain | Arduino CLI 1.5.1, Arduino SAMD 1.8.14, TinyCircuits SAMD 1.1.0, TinyScreen 1.1.0, Arduino Keyboard 1.0.7, and TinyCircuits STBLE 0.1.0 |

## Set up the TraceHub server

On the server, open PowerShell in the repository root and install the dependencies once.

```powershell
$traceDeckEnvironment = Join-Path $env:LOCALAPPDATA 'TraceDeck\venv'
py -3.11 -m venv $traceDeckEnvironment
$traceDeckPython = Join-Path $traceDeckEnvironment 'Scripts\python.exe'
& $traceDeckPython -m pip install -r .\tracehub\requirements.txt
```

### Ops Console

```powershell
$traceDeckPython = Join-Path $env:LOCALAPPDATA 'TraceDeck\venv\Scripts\python.exe'
& $traceDeckPython -m flask --app tracehub --no-debug run --host 127.0.0.1
```

On the server, open `http://127.0.0.1:5000`. The workspace navigation opens the Loot Log, Walk-away audit, or Reverse Shell console. The overview summarizes captures, source IPs, identified hosts, and known upload sizes. Search, filter by hostname status, or order captures newest or oldest first, then open a row for the full file text and metadata. The log shows up to 50 matches and refreshes every two seconds.

### Browser upload listener

In a second PowerShell window on the server, bind the listener to the server's private IPv4 address on the demo network.

```powershell
$traceDeckPython = Join-Path $env:LOCALAPPDATA 'TraceDeck\venv\Scripts\python.exe'
$traceHubServerIp = Read-Host 'TraceHub server private IPv4 address'
& $traceDeckPython -m flask --app "tracehub:create_app('badusb_demo')" --no-debug run --host $traceHubServerIp --port 8080
```

The upload build sends `Test_Important.zip` to `http://<server-ip>:8080/badusb/upload`. The page also accepts a manual ZIP selection. ZIP entries must be inside `Test_Important`. The request limit is 8 MiB, with at most 100 archive entries and 16 MiB of expanded data. TraceHub saves text previews and metadata in SQLite and keeps the original ZIP under `%LOCALAPPDATA%\TraceDeck\archives`. Open a Loot Log row and select **Download ZIP folder** to save the original folder on your PC, including PDFs, Word documents, and other files. Text is shown in the preview. Other document types show a download notice. TraceHub records the observed source IP and receipt time.

PowerShell prints the loot ID after TraceHub saves the ZIP and database row, then deletes the temporary ZIP on the target. On failure, it keeps the temporary ZIP and prints its path. The original `Documents\Test_Important` folder stays on the target. TraceHub automatically adds archive metadata support when it opens an older receipts database.

Expand a Loot Log entry to see image previews from its saved ZIP. JPG/JPEG, PNG, GIF, and WebP images appear below the text preview with their archive filenames. Click an image to open it in a new tab. Previews also work for earlier uploads whose original ZIP is still stored.

The listeners share `%LOCALAPPDATA%\TraceDeck\tracehub.sqlite3`. To start with an empty console, stop all TraceHub listeners and rename the database to a backup. TraceHub creates a new database on restart.

### Optional authenticated HTTPS receiver

Start this separate listener on the server if an enrolled client needs to submit files to `POST /api/v1/loot`. Store helper tokens and the TLS private key in the server's local application data.

```powershell
$traceDeckData = Join-Path $env:LOCALAPPDATA 'TraceDeck'
$traceDeckPython = Join-Path $traceDeckData 'venv\Scripts\python.exe'
$credentialsFile = Join-Path $traceDeckData 'helpers.json'
if (-not (Test-Path -LiteralPath $credentialsFile)) {
    $helperToken = & $traceDeckPython -c "import secrets; print(secrets.token_urlsafe(32))"
    @{ 'lab-helper' = $helperToken } | ConvertTo-Json | Set-Content -LiteralPath $credentialsFile -Encoding utf8
}

$traceHubServerIp = Read-Host 'TraceHub server private IPv4 address'
$gitRoot = Split-Path (Split-Path (Get-Command git).Source)
$openssl = Join-Path $gitRoot 'usr\bin\openssl.exe'
$receiverCertificate = Join-Path $traceDeckData 'intake-cert.pem'
$receiverPrivateKey = Join-Path $traceDeckData 'intake-key.pem'
if (-not (Test-Path -LiteralPath $receiverCertificate) -or -not (Test-Path -LiteralPath $receiverPrivateKey)) {
    & $openssl req -x509 -newkey rsa:2048 -noenc -days 30 -subj '/CN=TraceHub server' -addext "subjectAltName=IP:$traceHubServerIp" -addext 'basicConstraints=critical,CA:FALSE' -addext 'extendedKeyUsage=serverAuth' -keyout $receiverPrivateKey -out $receiverCertificate
}
& $traceDeckPython -m flask --app "tracehub:create_app('loot_receiver')" --no-debug run --host $traceHubServerIp --port 8443 --cert $receiverCertificate --key $receiverPrivateKey
```

Regenerate the certificate when the server's IP changes or the certificate expires. Give each enrolled helper its own bearer token and the server's public certificate for TLS validation. The multipart request contains a UUID `submission_id`, a client-reported `hostname`, and one UTF-8 `.txt` file up to 1 KiB. A new upload returns `201` with a loot ID. An identical retry returns `200` with the same ID. A conflicting retry returns `409`. TraceHub displays the hostname reported by the authenticated helper. Browser uploads show an unknown hostname.

On Home, use R1 or R2 to move between BadUSB and Bluetooth. The cyan pointer and green text mark the selected row. Press L1 to open it.

In Payloads, the amber dash marks the selection. Cyan dots mark other visible items and gray dots mark offscreen items. Lists longer than six items share the same six positions.

## Build the handheld

On a new Windows x64 laptop, install the tested CLI with `winget install --id ArduinoSA.CLI --exact --version 1.5.1` and reopen PowerShell. Install the board packages and libraries once. [Arduino CLI installation](https://docs.arduino.cc/arduino-cli/installation/) · [TinyScreen+ setup](https://learn.tinycircuits.com/Processors/TinyScreen%2B_Setup_Tutorial/)

```powershell
arduino-cli config add board_manager.additional_urls https://files.tinycircuits.com/ArduinoBoards/package_tinycircuits_index.json
arduino-cli core update-index
arduino-cli core install "arduino:samd@1.8.14" "TinyCircuits:samd@1.1.0"
arduino-cli lib update-index
arduino-cli lib install "TinyScreen@1.1.0" "Keyboard@1.0.7"
```

Install the TinyCircuits STBLE 0.1.0 ZIP in the Arduino sketchbook. In Arduino IDE, use **Sketch → Include Library → Add .ZIP Library**. The commands below install it in the Arduino CLI's configured sketchbook. If you also use Arduino IDE, select that same sketchbook in its preferences. [TinyCircuits STBLE archive](https://github.com/TinyCircuits/TinyCircuits-TinyShield-BLE-ASD2116/tree/2dcc1d42c2e63ca9d489355665cf2769600d94d8/examples)

```powershell
$arduinoLibraries = Join-Path (arduino-cli config get directories.user) 'libraries'
New-Item -ItemType Directory -Path $arduinoLibraries -Force | Out-Null
$stbleArchive = Join-Path $env:TEMP 'TraceDeck-STBLE-0.1.0.zip'
$stbleSource = 'https://raw.githubusercontent.com/TinyCircuits/TinyCircuits-TinyShield-BLE-ASD2116/2dcc1d42c2e63ca9d489355665cf2769600d94d8/examples/STBLE.zip'
Invoke-WebRequest -Uri $stbleSource -OutFile $stbleArchive
Expand-Archive -LiteralPath $stbleArchive -DestinationPath $arduinoLibraries
arduino-cli lib list
```

Connect the board, switch it on, and choose its COM port from `arduino-cli board list`. The default build saves the test file locally.

```powershell
arduino-cli board list
$traceDeckPort = Read-Host 'TinyScreen+ COM port'
arduino-cli compile --fqbn TinyCircuits:samd:tinyscreen --port $traceDeckPort --upload .\firmware\TraceDeck
```

For the upload build, enter the server IP used for the browser listener. Use the command below for the current firmware. Keep the configured firmware image local.

```powershell
$traceHubServerIp = Read-Host 'TraceHub server private IPv4 address'
arduino-cli compile --fqbn TinyCircuits:samd:tinyscreen --build-property "compiler.cpp.extra_flags=-DTRACEHUB_SERVER_IP=$traceHubServerIp" --port $traceDeckPort --upload .\firmware\TraceDeck
```

With the OLED upright, L1 and R1 are the upper buttons. L2 and R2 are the lower buttons.

## Run Drop & Upload

1. On Home, press L1 to open BadUSB, then press L1 on **Drop & Upload**. R1 and R2 scroll through the payload list. De-Shield and Pwning show a disabled screen when opened. L2 goes back from the payload list.
2. The payload opens in **Passive · Disarmed**. R1 toggles Passive and Active while disarmed. R2 arms the selected mode for 60 seconds. L2 cancels while armed and goes back while disarmed.
3. Prepare `%USERPROFILE%\Documents\Test_Important` with dummy files. In Passive, connect the armed TinyScreen+ to the target laptop and press R1 (**INJECT**). Check that `%USERPROFILE%\Desktop\TinyCircuitDemo.txt` contains `you have been pwned by TraceDeck` and Notepad closes. In the upload build, PowerShell creates a ZIP in a unique temporary folder, uploads it, and deletes that temporary ZIP after a valid acknowledgement. Match the loot ID in PowerShell to the Ops Console entry, inspect the text preview, and download the ZIP to verify the other documents.
4. Move the previous Desktop message aside before repeating to avoid an overwrite prompt. Check navigation, cancel, expiry, and held buttons. A held INJECT should run the sequence once. Cancel and expiry should prevent uploads. The OLED result describes the keyboard sequence. PowerShell and the saved loot ID establish the upload outcome, which may arrive after the board finishes sending keys.

### Check Active mode

Arm Active mode on battery and observe `WAIT USB`. A newly enumerated USB connection consumes the arm and executes the selected payload once. A disabled payload opens the disabled screen. When frames resume without observed re-enumeration, the screen shows `REPLUG USB` and keeps the original deadline. Verify one execution on connection, then verify that reconnecting while disarmed produces no output and that cancellation and expiry prevent execution.

Check battery plug-in, connected startup, reconnect without reboot, and sleep or wake with the cable attached. A wake that re-enumerates can trigger an armed Active payload, so include that case in physical testing. [TinyScreen+ schematic](https://github.com/TinyCircuits/TinyCircuits-TinyScreenPlus-ASM2022/blob/master/design_files/ASM2022.sch)

## Run BLE Nearby

For a button-by-button walkthrough, laptop setup, two-board radio check and Windows lock trial, use the [TraceDeck Bluetooth guide](BLUETOOTH_GUIDE.md).

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
   & $python -m flask --app "tracehub:create_app('walk')" run --host 127.0.0.1 --port 5074
   ```

4. In a second PowerShell window set the same `$python` variable. Run the manual control, press Win+L, then sign in. Refresh the trial view and verify **Manual control observed**. Use a fresh trial for the actual walk.

   ```powershell
   & $python -m tracehub.walk_capture control
   & $python -m tracehub.walk_capture walk --port COM8 --route "Service desk to doorway" --configuration "Record actual phone pairing, Dynamic Lock, sleep and idle settings"
   ```

5. Use the handheld's identified application port. Arming requires a desk-beacon observation less than ten seconds old. When the OLED shows **Ready to mark**, leave the station idle and carry the paired phone and the battery-powered handheld. At the first point where the keyboard is out of sight, press **L1** once. Confirm **Doorway marked**. Remain there for at least 90 seconds to measure the full window. Return, reconnect the handheld and allow the capture to finish. On **Trial saved**, refresh TraceHub. A marked trial remains in RAM until the PC acknowledges the save. To abandon one deliberately, press **L2**, then **L1** to confirm or **L2** to keep it.
6. TraceHub stores the button time, latest desk-beacon signal and age at the button press, Windows lock time, result and timing uncertainty. It uses the same assessment PC for both clock exchanges and the local WTS event observer. Doorway time is mapped from the board tick to the PC clock. Windows times are local receipt times for session notifications. Displayed uncertainty covers clock alignment. Windows scheduling can add notification delay. Keep the setting and the observed outcome together when interpreting a run.

Results include **Lock observed**, **Already locked at marker**, **No lock observed within 90 s**, or **Inconclusive** with a reason. A negative finding requires continuous observer coverage. A stale radio reading is valid at the doorway. A missing observer, session change, sleep, board reset, or ambiguous clock order can make the result inconclusive. Completed journals are in `%LOCALAPPDATA%\TraceDeck\walk-trials\<trial-id>.json`. Re-import a finalized journal with `python -m tracehub.walk_capture import "<file>"`. Identical imports keep one database entry. Refresh the console to see the result after restart.

For the assessment, film the marked doorway, the physical OLED confirmation and the matching trial ID in TraceHub. Repeat the same route with the paired phone left on the desk as a comparison. Show the measured values and identify the source of each timestamp.

## Repository layout

- `firmware/TraceDeck/` contains the Arduino sketch, shared UI and input code, BadUSB under `src/badusb/`, and the scanner, walk-away audit and desk beacon under `src/bluetooth/`.
- `tracehub/` contains the Flask factory, listener modules, shared SQLite storage, templates, and the redesigned console assets on main.

Record build checks and physical rehearsal results separately. Preserve upstream licenses and attribute reused material in the source and video.
