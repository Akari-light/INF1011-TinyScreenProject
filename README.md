# TraceDeck

TraceDeck is an ICT1011 TinyCircuits security lab. Two TinyScreen+ nodes run controlled USB keyboard and Wi-Fi interactions against a laptop-hosted IoT decoy. A live timeline links each lab run to its recorded events.

The TinyScreen+ USB keyboard path has passed the Windows Notepad test. The firmware opens the USB Test screen at startup.

See [PROJECT_PLAN.md](PROJECT_PLAN.md) for requirements, architecture decisions, responsibilities, milestones, task status, and team working conventions.

## Hardware and software

| Item | Current value |
| --- | --- |
| USB keyboard node | TinyScreen+ ASM2022 with a USB data cable |
| Wi-Fi node | TinyScreen+ ASM2022 with the borrowed ASD2123-R Wi-Fi TinyShield |
| Lab host | Team laptop running the input app, IoT decoy, and dashboard on a private 2.4 GHz network |
| USB toolchain | Arduino CLI 1.5.1, Arduino SAMD 1.8.14, TinyCircuits SAMD 1.1.0, TinyScreen 1.1.0, Arduino Keyboard 1.0.7 |

## Setup and operation

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

1. Power the board on and confirm the OLED reads `USB Test`, `SAFE`, and `Ready`.
2. Tap and release L2 (`ARM`). Confirm `ARMED` appears.
3. Within 10 seconds, tap R2 (`CANCEL`). Confirm `SAFE` and `Cancelled` appear.
4. Arm again and wait 10 seconds. Confirm `SAFE` and `Timed out` appear.
5. Hold L2 through a timeout. Confirm the screen stays `SAFE` until you release the button and press it again.

On 8 Nov 2026, Arvin verified startup in SAFE after a power cycle, arming with L2, cancellation with R2, and return to SAFE after waiting for the arming window to expire. Holding L2 through timeout left the screen in SAFE.

### Check keyboard output in Notepad

After uploading TraceDeck, focus a blank Notepad document on the test laptop. Use the US keyboard layout and turn Caps Lock off. The marker is `tracedeck usb test` with no newline. The OLED reports `Send attempted` when the firmware calls the Arduino Keyboard library. Notepad shows what the laptop actually received. [Arduino Keyboard library](https://github.com/arduino-libraries/Keyboard/tree/1.0.7)

1. In `SAFE`, tap R1. Expect the document to stay blank.
2. Tap and release L2 (`ARM`), then tap R1 (`SEND`) within 10 seconds. Expect one exact marker and a return to `SAFE`.
3. Start with a blank document again. Tap and release L2, then press and hold R1 for 3 seconds. Expect one exact marker, including after releasing R1.
4. Start with a blank document. Arm with L2, cancel with R2, then tap R1. Expect the document to stay blank. Repeat with an expired arming window.

On 8 Nov 2026, Arvin confirmed the four-button orientation and passed these checks on the TinyScreen+ and a Windows laptop. Tapping or holding R1 after arming entered exactly one marker and returned the screen to SAFE. R1 produced no text when unarmed, cancelled, or expired.

Arvin repeated the SAFE startup and unarmed, held SEND, cancel, and timeout checks on the same setup after the button debounce was moved into a shared reader. All passed on 8 Nov 2026.

## Repository layout

The USB Test sketch is in `firmware/TraceDeck/`. Add the remaining directories when their first real files are needed.

| Directory | Purpose |
| --- | --- |
| `firmware/` | Device firmware and the selected toolchain's build configuration |
| `hardware/` | Component list, pin assignments, wiring diagrams, and editable design files |
| `tests/` | System test procedures and selected reproducible evidence |
| `docs/` | Supporting diagrams, source acknowledgements, and demo/submission material |

Use the chosen toolchain's native layout inside `firmware/`. Add a directory for the companion application when laptop development begins.

## Verification and reused material

The project plan lists the acceptance checks. Record build results separately from physical USB and Wi-Fi test results.

Preserve upstream license notices. Attribute reused or adapted code in the relevant source files and delivery material. Record source URLs, versions or revisions, and modifications. A project-wide license is pending.
