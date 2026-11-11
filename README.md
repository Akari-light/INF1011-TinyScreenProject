# TraceDeck

TraceDeck is an ICT1011 security lab for owned equipment on an isolated network. In **Drop & Upload**, the TinyScreen+ acts as a USB keyboard on the **Windows target laptop**. It creates a synthetic text file and, in the upload build, submits it through the laptop's browser. The **TraceHub server** stores the upload and displays it in the Ops Console.

See [PROJECT_PLAN.md](PROJECT_PLAN.md) for the roadmap and test status.

## Hardware and software

| Component | Setup |
| --- | --- |
| USB node | TinyScreen+ ASM2022, Micro-USB data cable, and a charged battery for the USB diagnostic |
| TraceHub server | Windows with Python 3.11 to run the upload listener and Ops Console |
| Target laptop | Owned, unlocked Windows laptop with Notepad, a default browser, English (US) keyboard layout, and Caps Lock off |
| Arduino toolchain | Arduino CLI 1.5.1, Arduino SAMD 1.8.14, TinyCircuits SAMD 1.1.0, TinyScreen 1.1.0, and Arduino Keyboard 1.0.7 |

## Set up the TraceHub server

On the server, use a checkout with the redesigned TraceHub console (`main` at `4949912` or later). Open PowerShell in the repository root and install the dependencies once.

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

On the server, open `http://127.0.0.1:5000`. The overview summarizes uploads and source devices. Search, filter, or sort the Loot Log, then open a row for the full file text and metadata. The log shows up to 50 matches and refreshes every two seconds.

### Browser upload listener

In a second PowerShell window on the server, bind the listener to the server's private IPv4 address on the demo network.

```powershell
$traceDeckPython = Join-Path $env:LOCALAPPDATA 'TraceDeck\venv\Scripts\python.exe'
$traceHubServerIp = Read-Host 'TraceHub server private IPv4 address'
& $traceDeckPython -m flask --app "tracehub:create_app('badusb_demo')" --no-debug run --host $traceHubServerIp --port 8080
```

The target laptop opens `http://<server-ip>:8080/badusb/upload`. The listener accepts only `hello-world.txt` containing `hello world` with an optional final newline. TraceHub records the source IP it observes and the receipt time. The upload page and Ops Console show the same loot ID. Retrying with the same form and file returns that ID.

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

## Build and run Drop & Upload

Install the board packages and libraries once. [Arduino CLI installation](https://docs.arduino.cc/arduino-cli/installation/) · [TinyScreen+ setup](https://learn.tinycircuits.com/Processors/TinyScreen%2B_Setup_Tutorial/)

```powershell
arduino-cli config add board_manager.additional_urls https://files.tinycircuits.com/ArduinoBoards/package_tinycircuits_index.json
arduino-cli core update-index
arduino-cli core install arduino:samd`1.8.14
arduino-cli core install TinyCircuits:samd`1.1.0
arduino-cli lib install TinyScreen`1.1.0
arduino-cli lib install Keyboard`1.0.7
```

Connect the board, switch it on, and choose its COM port from `arduino-cli board list`. The default build saves the test file locally.

```powershell
arduino-cli board list
$traceDeckPort = Read-Host 'TinyScreen+ COM port'
arduino-cli compile --fqbn TinyCircuits:samd:tinyscreen --port $traceDeckPort --upload .\firmware\TraceDeck
```

For the upload build, enter the server IP used for the browser listener. Use the command below for the current firmware. If you build firmware from `4949912`, use its compile flag `TRACEHUB_PC1_IP` in place of `TRACEHUB_SERVER_IP`. Keep the configured firmware image local.

```powershell
$traceHubServerIp = Read-Host 'TraceHub server private IPv4 address'
arduino-cli compile --fqbn TinyCircuits:samd:tinyscreen --build-property "compiler.cpp.extra_flags=-DTRACEHUB_SERVER_IP=$traceHubServerIp" --port $traceDeckPort --upload .\firmware\TraceDeck
```

With the OLED upright, L1 and R1 are the upper buttons. L2 and R2 are the lower buttons.

1. On Home, press L1 to open BadUSB, then press L1 on **Drop & Upload**. R1 and R2 move through the payload list. The two `SOON` slots show a dimmed OPEN control. L2 goes back.
2. The payload opens in **Passive · Disarmed**. R1 toggles Passive and Active while disarmed. R2 arms the selected mode for 60 seconds. L2 cancels while armed and goes back while disarmed.
3. In Passive, connect the armed TinyScreen+ to the target laptop and press R1 (**INJECT**). The OLED shows progress, then `KEYS SENT` if USB remains configured. For the default build, check `%TEMP%\hello-world.txt` on the target laptop. For the upload build, match the browser's loot ID to the receipt in the Ops Console.
4. Move the previous demo file out of `%TEMP%` on the target laptop before repeating to avoid a replacement prompt. Check navigation, cancel, expiry, and held buttons. A held INJECT should run the sequence once. Cancel and expiry should prevent uploads.

### Check the Active preview

Active currently reports USB connection events on the OLED. Arm on battery and observe `WAIT USB`. A new configured connection can produce `USB seen (test)` and disarm the board. When frames resume without observed re-enumeration, the screen shows `REPLUG USB` and keeps the original deadline. Keyboard output requires Passive mode and a manual INJECT.

For a dedicated diagnostic, flash `firmware/diagnostics/UsbConnectionProbe` with the same board and port flags. R2 starts a 60-second watch and L2 clears the result. Check battery plug-in, connected startup, reconnect without reboot, and sleep or wake with the cable attached. Record `ENUM + LIVE` or `LIVE NO ENUM`. A wake that re-enumerates can resemble a fresh plug-in, so this check gates Active output. Reflash TraceDeck after using the probe. [TinyScreen+ schematic](https://github.com/TinyCircuits/TinyCircuits-TinyScreenPlus-ASM2022/blob/master/design_files/ASM2022.sch)

## Repository layout

- `firmware/TraceDeck/` contains the Arduino sketch, shared UI and input code, and the BadUSB payloads under `src/badusb/payloads/`.
- `firmware/diagnostics/` contains the USB connection probe.
- `tracehub/` contains the Flask factory, listener modules, shared SQLite storage, templates, and the redesigned console assets on main.

Record build checks and physical rehearsal results separately. Preserve upstream licenses and attribute reused material in the source and video.
