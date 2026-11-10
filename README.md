# TraceDeck

TraceDeck is an ICT1011 TinyCircuits security lab. The planned USB and Wi-Fi paths use two TinyScreen+ nodes. TraceHub receives lab evidence on PC1, and the Ops Console displays the saved loot as it arrives.

The TinyScreen+ starts on Home with `BadUSB` selected and `WI-FI SOON` as a placeholder. Open BadUSB to reach the payload menu and select `Drop & Upload`. Arvin confirmed that the Notepad sequence saved `hello-world.txt` in `%TEMP%` on PC2.

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

To start with an empty Ops Console, stop both TraceHub processes on PC1 and rename `tracehub.sqlite3` to a backup. Starting TraceHub again creates an empty database.

### Start the BadUSB demo upload on PC1

In a second PowerShell window in the project root, run `ipconfig` and find PC1's private IPv4 address on the isolated demo network. Use that address to bind the upload listener to the demo interface.

```powershell
$traceDeckPython = Join-Path $env:LOCALAPPDATA 'TraceDeck\venv\Scripts\python.exe'
$pc1Ip = Read-Host 'PC1 private demo-network IPv4 address'
& $traceDeckPython -m flask --app "tracehub:create_app('badusb_demo')" --no-debug run --host $pc1Ip --port 8080
```

The PC2 browser opens `http://<PC1-private-IP>:8080/badusb/upload`. The page focuses the file picker and automatically submits a UTF-8 `hello-world.txt` containing only `hello world`, with or without a final line break. This is a cleartext HTTP profile for the fixed synthetic file. If Windows Firewall needs a rule, scope it to the isolated demo network.

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

### Build and check BadUSB controls

With the OLED text upright, the upper buttons are L1 and R1, and the lower buttons are L2 and R2.

Use the board's current COM port for `$port`.

```powershell
$port = 'COM3'
arduino-cli compile --fqbn TinyCircuits:samd:tinyscreen .\firmware\TraceDeck
arduino-cli compile --fqbn TinyCircuits:samd:tinyscreen --port $port --upload .\firmware\TraceDeck
```

Those commands build the Notepad-only version of HID Drop & Upload. That version runs the local save step as a test. To include the browser upload in the same payload, use the same private IPv4 address as the PC1 demo listener on port 8080. Enter the address at build time so it stays in the local firmware image and out of shared source. Keep the configured firmware image local.

```powershell
$pc1Ip = Read-Host 'PC1 private demo-network IPv4 address'
arduino-cli compile --fqbn TinyCircuits:samd:tinyscreen --build-property "compiler.cpp.extra_flags=-DTRACEHUB_PC1_IP=$pc1Ip" --port $port --upload .\firmware\TraceDeck
```

1. Power the board on and confirm Home highlights `BadUSB`. Tap L1 (`OPEN`) to reach `PAYLOADS`. Confirm it lists `Drop & Upload`, `Payload 2`, and `Payload 3`. The last two rows show `SOON`.
2. With `Drop & Upload` selected, tap L1 again. Confirm the screen shows `PASSIVE`, a green shield, `DISARMED`, and `Standby`. R1 changes modes while disarmed. The underline beneath `P` or `A` marks the selected mode. The build commands above determine whether this payload includes browser upload.
3. In Passive, tap and release R2 (`ARM`). Confirm `ARMED`, `60s`, and ten full amber segments appear. At `59s`, the rightmost segment should show five-sixths fill.
4. Within 60 seconds, tap L2 (`CANCEL`). Confirm the gauge clears and the green shield and `Cancelled` appear.
5. Arm again and wait 60 seconds. Confirm the last segment stays lit through `01s`, then the gauge clears and the green shield and `Timed out` appear.
6. Hold R2 through a timeout. Confirm the screen stays disarmed until you release the button and press it again.
7. While disarmed, tap L2 (`BACK`) to return to the payload menu. Release L2 and tap it again to return to Home.

On 8 Nov 2026, Arvin verified startup in SAFE on the original standalone USB Test screen, arming with L2, cancellation with R2, and return to SAFE after waiting for the arming window to expire. Holding L2 through timeout left the screen in SAFE.

### Check menu navigation in Notepad

Focus a blank Notepad document for all of these checks. Expect it to stay blank throughout.

1. On Home, tap R1 (up arrow) twice. The selection pointer should move to `WI-FI SOON` and then back to `BadUSB`. Repeat with R2 (down arrow). Both directions wrap.
2. Highlight `WI-FI SOON` and tap L1 (`OPEN`). Home should stay visible with the same selection.
3. Highlight `BadUSB`. Hold L1 and confirm it stops at `PAYLOADS`. Release L1. Use R1 and R2 to cycle up and down through all three rows, including wraparound. On `Payload 2` and `Payload 3`, verify that `OPEN` dims and L1 stays on the menu.
4. Return to `Drop & Upload` and hold L1 to open it disarmed. With L1 still held, also hold R2. Release L1 while keeping R2 held for a second. The screen should keep showing the green shield. Release R2.
5. Tap R2 to arm, then hold L2 (`CANCEL`). The execution screen should return to the green shield. Release L2 and hold it again (`BACK`). The payload menu should open with `Drop & Upload` still selected and stay there while you hold L2. Release L2 and tap it again to return to Home with `BadUSB` selected.

On 8 Nov 2026, Arvin verified both wrap directions, the Wi-Fi placeholder, held-button transitions, and cancel followed by Back on the TinyScreen+ with the original USB Test labels. Notepad stayed blank throughout.

### Check the OLED theme

On Home, check that the cyan pointer and brighter text clearly mark the selected row. The USB and Wi-Fi icons identify the two operations. A short divider separates the violet `OPEN` control from the larger green `HOME` title. The violet arrows mark the right-hand navigation buttons. `SOON` stays gray, and `OPEN` dims when Wi-Fi is selected.

Open BadUSB and check that the three payload rows, selection pointer, and `SOON` labels are legible. Open `Drop & Upload` and check that the underline moves between P and A with each R1 press. The checked shield marks the disarmed screen. In Passive, arm with R2 and check the warning triangle, amber `ARMED` label, and seconds readout. The ten slanted segments each represent six seconds. The rightmost lit segment loses one-sixth of its fill on every tick, then stays visible as a dim outline. The readout and lit segments fade from amber to red-orange. From `06s` through `01s`, the last segment and the digits gently dim and brighten. Check that the final one-sixth fill and `01s` stay legible at the dimmest point.

On 8 Nov 2026, Arvin checked the icons and countdown on the TinyScreen+ and confirmed the final header separation, muted Wi-Fi text, and saturated green on the physical OLED. The final TinyScreen+ build used 30,900 bytes and passed upload verification.

### Check the BadUSB Notepad save

With the owned PC2 laptop unlocked, use the English (US) keyboard layout with Caps Lock off. The board opens Windows Run, starts Notepad, opens a blank document, types `hello world`, and sends Save As keystrokes for `%TEMP%\hello-world.txt`. The OLED shows `INJECTING` with a spinner and a bar that fills across the timed steps, then `KEYS SENT` after the board issues the sequence. Check the file on PC2 to confirm the save. [Arduino Keyboard library](https://github.com/arduino-libraries/Keyboard/tree/1.0.7)

1. While disarmed, tap R1 twice. Confirm the mode changes to Active and then back to Passive while PC2 stays on its current screen.
2. In Passive, arm with R2, cancel with L2, then tap R1 twice. Confirm the screen remains disarmed and PC2 stays on its current screen. Repeat after letting the arming window expire.
3. In Passive, tap and release R2 (`ARM`), then tap R1 (`INJECT`) within 60 seconds. Confirm the expected file exists and contains exactly `hello world`.
4. Move the demo file aside before another save test. Arm again and hold R1 through the sequence. Check that PC2 receives just one sequence and that the board returns to `SAFE`.

If the destination file already exists, Windows may show a replacement prompt. The sequence ends at the Save action and leaves the prompt for the operator.

The 10 Nov 2026 TinyScreen+ build used 31,224 bytes and passed flash verification. Arvin confirmed that the board sequence saved `%TEMP%\hello-world.txt` on PC2. The exact file contents, OLED feedback, and button regression checks are pending for this build. The 8 Nov 2026 physical checks passed for the prior `tracedeck usb test` marker, including unarmed, cancelled, expired, and held-SEND cases.

### Check USB plug-in detection on battery

`UsbConnectionProbe` is a separate status-only sketch for the Active review checkpoint. It uses the same USB keyboard descriptor as TraceDeck and sends zero keyboard reports. Use the board's current COM port and a charged battery. The upload replaces the sketch on the board.

```powershell
$port = 'COM6'
arduino-cli compile --fqbn TinyCircuits:samd:tinyscreen --port $port --upload .\firmware\diagnostics\UsbConnectionProbe
```

The OLED shows recent USB frames, `CFG` from the Arduino core, and the assigned USB `ADDR`. A device can retain its configuration and address when frames stop. R2 (`WATCH`) opens a 60-second observation window after startup settles and frames have been quiet for 250 ms. The probe waits for an addressed, configured link with continuing frames for 150 ms. `ENUM + LIVE` means the probe also saw the device unaddressed or unconfigured before that link. `LIVE NO ENUM` means frames returned without an observed loss of either. Both are observations about the USB bus. A wake that re-enumerates can look like a plug-in to this probe. [SAM D21 frame register](https://onlinedocs.microchip.com/oxy/GUID-22527069-B4D6-49B9-BACC-3AF1C52EB48C-en-US-21/GUID-BFF4D0A7-CEE1-4138-BA96-BF75A1E70463.html) · [TinyScreen+ schematic source](https://github.com/TinyCircuits/TinyCircuits-TinyScreenPlus-ASM2022/blob/master/design_files/ASM2022.sch)

The probe shows `WATCHING USB` during the observation window. Keep the serial monitor closed during these tests. With the board connected, a serial client can send `?` at 115200 baud to read one snapshot. L2 (`CLEAR`) clears the latched watch result.

1. **Startup connected.** With the cable attached to an awake PC2, restart the board. After it enumerates, confirm `FRAMES LIVE`, `CFG 1`, a nonzero address, and `IDLE`. Tap R2 and confirm `WAIT FOR QUIET`.
2. **First plug-in.** Unplug and restart the board on battery alone. Wait at least two seconds, tap R2, and connect the data cable to awake PC2. Record the initial `CFG` and `ADDR` and the resulting watch label. Expect `ENUM + LIVE`.
3. **Reconnect.** Unplug while leaving the board running on battery. Confirm `FRAMES QUIET` and record whether `CFG` and `ADDR` stayed set. Tap R2 and reconnect to awake PC2. Record the watch label again. Expect `ENUM + LIVE`.
4. **Sleep and wake.** Leave the cable plugged in and sleep PC2 with the board running on battery. If frames go quiet, tap R2 and wake PC2 within 60 seconds. Record `LIVE NO ENUM`, `ENUM + LIVE`, or any other result. If frames never go quiet, record that the trial did not exercise a quiet-to-live transition. Repeat any ambiguous case before reviewing Active output.

Bring the observed labels back to the review checkpoint. A wake result of `ENUM + LIVE` blocks using that result as a fresh-plug trigger. After the diagnostic, reflash TraceDeck with the normal build commands above.

### Check the Active waiting screen

Open `Drop & Upload`, press R1 to select A, and press R2 (`ARM`). The screen shows `WAIT USB`, a USB plug tile with animated signal bars, and the 60-second countdown. L2 cancels, and the mode selector returns when disarmed.

This checkpoint uses Active for status only. With the board running on battery, try arming while already connected to an awake PC2. The existing connection should leave the screen waiting. Unplug the data cable, wait a second, and reconnect before expiry. The probe outcome `ENUM + LIVE` appears here as `DISARMED` and `USB seen (test)`. Repeat with PC2 sleep and wake while leaving the cable attached. A return of frames without re-enumeration shows `REPLUG USB` and preserves the original deadline. A `USB seen (test)` result from wake leaves Active keyboard output gated. Cancel, expiry, and the Active test path should leave a blank Notepad document on PC2 unchanged.

## Repository layout

The TraceDeck sketch is in `firmware/TraceDeck/`. Add the remaining directories when their first real files are needed.

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

For the integrated BadUSB check, start the Ops Console and demo listener on PC1, then connect the configured board to the unlocked PC2 laptop on the isolated demo network. Move aside any existing `%TEMP%\hello-world.txt` before each fresh attempt so the save creates a new file. A deliberate injection should create the file, open the upload page in the default browser, and select the file. The page submits the selection automatically. Compare the loot ID shown on PC2 to the single new entry in the PC1 console and check its source IP, filename, upload size, and decoded text. Repeat with a fresh file. Check that an unarmed, cancelled, or expired attempt creates no entry and that holding INJECT produces only one.

The firmware waits 2.5 seconds after sending Save before opening the browser. It also waits 2.5 seconds for Notepad and the browser to open and 1.75 seconds at dialog transitions. If Windows displays a replacement prompt, move the existing demo file aside before retrying. That prompt requires a separate action to complete the save.

On 10 Nov 2026, the configured TinyScreen+ build with the revised waits used 31,356 bytes and passed flash verification. In a separate loopback Chrome check, Enter opened the focused file picker. Selecting the expected file saved one loot entry with the same ID on the upload page and dashboard. Selecting a file with different contents left the form open.

Arvin reported a successful deliberate PC2 injection on 10 Nov 2026. The PC1 database contained one browser-demo entry for `hello-world.txt` with 11 bytes, decoded text `hello world`, and a non-loopback source IP. The repeat and unarmed, cancelled, expired, and held-INJECT cases are pending for the integrated build.

Preserve upstream license notices. Attribute reused or adapted code in the relevant source files and delivery material. Record source URLs, versions or revisions, and modifications. A project-wide license is pending.
