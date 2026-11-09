# TraceDeck

TraceDeck is an ICT1011 TinyCircuits security lab. The planned USB and Wi-Fi paths use two TinyScreen+ nodes. TraceHub receives lab evidence on PC1, and the Ops Console displays the saved loot as it arrives.

The TinyScreen+ USB keyboard path has passed the Windows Notepad test. The firmware starts on Home with `USB TEST` selected and `WI-FI SOON` as a placeholder.

See [PROJECT_PLAN.md](PROJECT_PLAN.md) for requirements, architecture decisions, responsibilities, milestones, task status, and team working conventions.

## Hardware and software

| Item | Current value |
| --- | --- |
| USB keyboard node | TinyScreen+ ASM2022 with a USB data cable |
| Wi-Fi node | TinyScreen+ ASM2022 with the borrowed ASD2123-R Wi-Fi TinyShield |
| PC1 | Windows lab PC hosting the Ops Console, BadUSB demo upload, and later mock IoT service |
| PC2 | Unlocked Windows lab laptop with Notepad and a browser on the isolated demo network |
| USB toolchain | Arduino CLI 1.5.1, Arduino SAMD 1.8.14, TinyCircuits SAMD 1.1.0, TinyScreen 1.1.0, Arduino Keyboard 1.0.7 |

## Setup and operation

### Start the TraceDeck Ops Console on PC1

Use Python 3.11 and open PowerShell in the project root. The virtual environment and SQLite database live in the current user's local application data.

```powershell
$traceDeckEnvironment = Join-Path $env:LOCALAPPDATA 'TraceDeck\venv'
py -3.11 -m venv $traceDeckEnvironment
$traceDeckPython = Join-Path $traceDeckEnvironment 'Scripts\python.exe'
& $traceDeckPython -m pip install -r .\tracehub\requirements.txt
& $traceDeckPython -m flask --app tracehub --no-debug run --host 127.0.0.1
```

Open `http://127.0.0.1:5000` on PC1. The dashboard checks for new loot every two seconds and shows the newest 50 entries. SQLite retains the saved loot across restarts in `%LOCALAPPDATA%\TraceDeck\tracehub.sqlite3`.

### Start the BadUSB demo upload on PC1

In a second PowerShell window in the project root, run `ipconfig` and find PC1's private IPv4 address on the isolated demo network. Use that address to bind the upload listener to the demo interface.

```powershell
$traceDeckPython = Join-Path $env:LOCALAPPDATA 'TraceDeck\venv\Scripts\python.exe'
$pc1Ip = Read-Host 'PC1 private demo-network IPv4 address'
& $traceDeckPython -m flask --app "tracehub:create_app('badusb_demo')" --no-debug run --host $pc1Ip --port 8080
```

The PC2 browser opens `http://<PC1-private-IP>:8080/badusb/upload`. The page accepts one UTF-8 `hello-world.txt` containing only `hello world`, with or without a final line break. This is a cleartext HTTP profile for the fixed synthetic file. If Windows Firewall needs a rule, scope it to the isolated demo network.

After saving the decoded text and metadata, TraceHub shows the loot ID on the upload page and in the Ops Console. Browser submissions show the peer IP PC1 observed. Retrying the same form with the same file returns the same loot ID. Open a fresh form for a new attempt. Stop the demo listener with Ctrl+C when finished.

### Start the separate HTTPS loot receiver on PC1

Use a second PowerShell window in the project root. Find PC1's private lab-network IPv4 address with `ipconfig`. Keep `helpers.json` and the TLS private key in the Flask instance directory under local application data.

```powershell
$traceDeckData = Join-Path $env:LOCALAPPDATA 'TraceDeck'
$traceDeckPython = Join-Path $traceDeckData 'venv\Scripts\python.exe'
$credentialsFile = Join-Path $traceDeckData 'helpers.json'
if (-not (Test-Path -LiteralPath $credentialsFile)) {
    $helperToken = & $traceDeckPython -c "import secrets; print(secrets.token_urlsafe(32))"
    @{ 'pc2-a' = $helperToken } | ConvertTo-Json | Set-Content -LiteralPath $credentialsFile -Encoding utf8
}
```

`helpers.json` maps each helper ID to its own generated token. Add a new entry and restart the loot receiver to enroll another helper. Transfer only the matching token and PC1's public certificate to each helper.

Create a certificate once for PC1's private address. The commands use OpenSSL bundled with Git for Windows. Generate a replacement certificate if the address changes or the certificate expires.

```powershell
$pc1Ip = Read-Host 'PC1 private IPv4 address'
$gitRoot = Split-Path (Split-Path (Get-Command git).Source)
$openssl = Join-Path $gitRoot 'usr\bin\openssl.exe'
$receiverCertificate = Join-Path $traceDeckData 'intake-cert.pem'
$receiverPrivateKey = Join-Path $traceDeckData 'intake-key.pem'
& $openssl req -x509 -newkey rsa:2048 -noenc -days 30 -subj '/CN=TraceHub PC1' -addext "subjectAltName=IP:$pc1Ip" -addext 'basicConstraints=critical,CA:FALSE' -addext 'extendedKeyUsage=serverAuth' -keyout $receiverPrivateKey -out $receiverCertificate
```

```powershell
& $traceDeckPython -m flask --app "tracehub:create_app('loot_receiver')" --no-debug run --host $pc1Ip --port 8443 --cert $receiverCertificate --key $receiverPrivateKey
```

The loot receiver exposes `POST /api/v1/loot` on the chosen private interface. A trusted HTTPS client uses the public certificate to validate PC1 and supplies `Authorization: Bearer <helper token>`. Generate a new UUID for each deliberate trigger and reuse it if you retry that submission.

An authenticated client sends `multipart/form-data` with two text fields, `submission_id` and `hostname`, and one uploaded part named `file`. Save the `.txt` file in UTF-8. The file limit is 1 KiB, and the full request can use up to 4 KiB to allow room for the form fields and multipart headers. PC1 saves the decoded text, filename, and original byte count in the loot entry.

A new submission returns HTTP 201 with `loot_id` after the save. The same ID appears in the Ops Console. An identical retry returns HTTP 200 and the original ID. Reusing a submission ID with different content or filename returns HTTP 409. A wrong token returns HTTP 401. A request with a different content type returns HTTP 415.

### Set up the TinyScreen+ on Windows

Open PowerShell in the project root. Install Arduino CLI, then run the following once to set up the board packages and display library. [Arduino CLI installation guide](https://docs.arduino.cc/arduino-cli/installation/) · [TinyCircuits setup guide](https://learn.tinycircuits.com/Processors/TinyScreen%2B_Setup_Tutorial/)

```powershell
arduino-cli config add board_manager.additional_urls https://files.tinycircuits.com/ArduinoBoards/package_tinycircuits_index.json
arduino-cli core update-index
arduino-cli core install arduino:samd@1.8.14
arduino-cli core install TinyCircuits:samd@1.1.0
arduino-cli lib install TinyScreen@1.1.0
arduino-cli lib install Keyboard@1.0.7
```

Connect the TinyScreen+ with a Micro-USB data cable and turn it on. Run `arduino-cli board list` and substitute the board's port for `COM3` below. Several TinyCircuits models can match the same port. The `--fqbn` flag selects the TinyScreen+ explicitly.

### Check the vendor baseline

Upload the example installed with the TinyScreen library. It replaces the sketch currently stored on the board.

```powershell
$port = 'COM3'
$sketchbook = arduino-cli config get directories.user | Select-Object -First 1
$example = Join-Path $sketchbook.Trim() 'libraries\TinyScreen\examples\TinyScreenBasicExample'
arduino-cli compile --fqbn TinyCircuits:samd:tinyscreen --port $port --upload $example
```

Check for `Verify successful` in the terminal and a repeating sequence of graphics and text on the OLED. Arvin observed both on a TinyScreen+ with Windows on 8 Nov 2026. [TinyScreen example source](https://github.com/TinyCircuits/TinyCircuits-TinyScreen_Lib/blob/master/examples/TinyScreenBasicExample/TinyScreenBasicExample.ino)

### Build and check USB Test

With the OLED text upright, the upper buttons are L1 and R1, and the lower buttons are L2 and R2.

Use the board's current COM port for `$port`.

```powershell
$port = 'COM3'
arduino-cli compile --fqbn TinyCircuits:samd:tinyscreen .\firmware\TraceDeck
arduino-cli compile --fqbn TinyCircuits:samd:tinyscreen --port $port --upload .\firmware\TraceDeck
```

1. Power the board on and confirm Home highlights `USB TEST`. Tap L1 (`OPEN`) and confirm the OLED reads `USB TEST`, `SAFE`, and `Ready`.
2. Tap and release L2 (`ARM`). Confirm `ARMED` appears.
3. Within 10 seconds, tap R2 (`CANCEL`). Confirm `SAFE` and `Cancelled` appear.
4. Arm again and wait 10 seconds. Confirm `SAFE` and `Timed out` appear.
5. Hold L2 through a timeout. Confirm the screen stays `SAFE` until you release the button and press it again.
6. In `SAFE`, tap R2 (`BACK`) to return to Home.

On 8 Nov 2026, Arvin verified startup in SAFE on the original standalone USB Test screen, arming with L2, cancellation with R2, and return to SAFE after waiting for the arming window to expire. Holding L2 through timeout left the screen in SAFE.

### Check Home navigation in Notepad

Focus a blank Notepad document for all of these checks. Expect it to stay blank throughout.

1. On Home, tap R1 (up arrow) twice. The selection pointer should move to `WI-FI SOON` and then back to `USB TEST`. Repeat with R2 (down arrow). Both directions wrap.
2. Highlight `WI-FI SOON` and tap L1 (`OPEN`). Home should stay visible with the same selection.
3. Highlight `USB TEST`. Hold L1 until USB Test opens in `SAFE`. With L1 still held, also hold L2. Release L1 while keeping L2 held for a second. The screen should stay `SAFE`. Release L2.
4. Tap L2 to arm, then hold R2 (`CANCEL`). The screen should return to `SAFE` and stay on USB Test. Release R2 and hold it again (`BACK`). Home should open and the selection should stay on `USB TEST` until a fresh press.

On 8 Nov 2026, Arvin verified both wrap directions, the Wi-Fi placeholder, held-button transitions, and cancel followed by Back on the TinyScreen+. Notepad stayed blank throughout.

### Check the OLED theme

On Home, check that the cyan pointer and brighter text clearly mark the selected row. The USB and Wi-Fi icons identify the two operations. A short divider separates the violet `OPEN` control from the larger green `HOME` title. The violet arrows mark the right-hand navigation buttons. `SOON` stays gray, and `OPEN` dims when Wi-Fi is selected.

Open USB Test and check the large green `SAFE` label and shield. Arm with L2 and check the amber `ARMED` label, warning icon, and countdown. Each amber tick represents one second. Let the countdown reach `01s` and confirm the display returns to `SAFE` with `Timed out`. The state label and controls should stay steady as the timer changes.

On 8 Nov 2026, Arvin checked the icons and countdown on the TinyScreen+ and confirmed the final header separation, muted Wi-Fi text, and saturated green on the physical OLED. The final TinyScreen+ build used 30,900 bytes and passed upload verification.

### Check keyboard output in Notepad

After uploading TraceDeck, open USB Test and focus a blank Notepad document on the test laptop. Use the US keyboard layout and turn Caps Lock off. The marker is `tracedeck usb test` with no newline. The OLED reports `Send attempted` when the firmware calls the Arduino Keyboard library. Notepad shows what the laptop actually received. [Arduino Keyboard library](https://github.com/arduino-libraries/Keyboard/tree/1.0.7)

1. In `SAFE`, tap R1. Expect the document to stay blank.
2. Tap and release L2 (`ARM`), then tap R1 (`SEND`) within 10 seconds. Expect one exact marker and a return to `SAFE`.
3. Start with a blank document again. Tap and release L2, then press and hold R1 for 3 seconds. Expect one exact marker, including after releasing R1.
4. Start with a blank document. Arm with L2, cancel with R2, then tap R1. Expect the document to stay blank. Repeat with an expired arming window.

On 8 Nov 2026, Arvin confirmed the four-button orientation and passed these checks on the TinyScreen+ and a Windows laptop. Tapping or holding R1 after arming entered exactly one marker and returned the screen to SAFE. R1 produced no text when unarmed, cancelled, or expired.

Arvin repeated the SAFE startup and unarmed, held SEND, cancel, and timeout checks on the same setup after the button debounce was moved into a shared reader. All passed on 8 Nov 2026.

On 8 Nov 2026, Arvin repeated all four Notepad checks with the icon and countdown layout. All passed. A one-marker SEND check also passed after the header and gray palette were tuned. The subsequent green-only adjustment was checked on the OLED.

## Repository layout

The USB Test sketch is in `firmware/TraceDeck/`. Add the remaining directories when their first real files are needed.

| Directory | Purpose |
| --- | --- |
| `firmware/` | Device firmware and the selected toolchain's build configuration |
| `tracehub/` | PC1 Ops Console, BadUSB demo upload, HTTPS loot receiver, and SQLite storage |
| `hardware/` | Component list, pin assignments, wiring diagrams, and editable design files |
| `tests/` | System test procedures and selected reproducible evidence |
| `docs/` | Supporting diagrams, source acknowledgements, and demo/submission material |

Use the chosen toolchain's native layout inside `firmware/`.

## Verification and reused material

The project plan lists the acceptance checks. Record build results separately from physical USB and Wi-Fi test results.

When the BadUSB firmware is ready, verify the file path on the actual lab devices. Use the PC as PC1 and the laptop as PC2. One physical injection should type `hello world` in Notepad, save `hello-world.txt` on the laptop, and submit it through the browser. Compare the loot ID on the upload page to the PC1 console entry and check its filename, upload size, and decoded text.

Preserve upstream license notices. Attribute reused or adapted code in the relevant source files and delivery material. Record source URLs, versions or revisions, and modifications. A project-wide license is pending.
