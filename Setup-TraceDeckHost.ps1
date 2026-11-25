#Requires -Version 5.1
<#
.SYNOPSIS
Set up the TraceDeck demo host end to end: install the Arduino and Python
dependencies, open the reverse-shell firewall port, flash the TinyScreen+,
and start TraceHub ready to receive files.

.PARAMETER ProjectPath
Path to the checkout. Defaults to the current working directory.

.PARAMETER InterfaceAlias
Network interface used to discover this host's LAN IPv4 address. Defaults to
"Wi-Fi". Use "auto" to prefer the interface that has a default gateway.

.PARAMETER HttpPort
HTTP port for TraceHub (default 8080).

.PARAMETER ShellPort
Raw TCP port for the reverse-shell listener and firewall rule (default 4444).

.PARAMETER SkipArduino
Skip Arduino CLI, board packages, and library setup.

.PARAMETER SkipFirewall
Skip the reverse-shell firewall rule.

.PARAMETER SkipFlash
Skip compiling and uploading the firmware.

.PARAMETER Port
Explicit serial port for the flash step (for example COM3). Defaults to
auto-detection through arduino-cli board list. Use this when the BLE shield
hides the board or board list cannot identify it.

.PARAMETER NoStart
Install and flash, then exit without starting TraceHub.
#>
[CmdletBinding()]
param(
    [string]$ProjectPath = $PSScriptRoot,
    [string]$InterfaceAlias = 'Wi-Fi',
    [ValidateRange(1024, 65535)]
    [int]$HttpPort = 8080,
    [ValidateRange(1024, 65535)]
    [int]$ShellPort = 4444,
    [switch]$SkipArduino,
    [switch]$SkipFirewall,
    [switch]$SkipFlash,
    [switch]$NoStart,
    [string]$Port = ''
)

$ErrorActionPreference = 'Stop'
$projectRoot = (Resolve-Path -LiteralPath $ProjectPath).ProviderPath
$requirements = Join-Path $projectRoot 'tracehub\requirements.txt'
$sketch = Join-Path $projectRoot 'firmware\TraceDeck'
if (-not (Test-Path -LiteralPath $requirements -PathType Leaf) -or
    -not (Test-Path -LiteralPath (Join-Path $projectRoot 'tracehub\__init__.py') -PathType Leaf) -or
    -not (Test-Path -LiteralPath (Join-Path $sketch 'TraceDeck.ino') -PathType Leaf)) {
    throw 'Supply the TraceDeck checkout directory with -ProjectPath.'
}

function Invoke-Checked {
    param([string]$Program, [string[]]$Arguments)

    & $Program @Arguments
    if ($LASTEXITCODE -ne 0) {
        throw "$([IO.Path]::GetFileName($Program)) failed with exit code $LASTEXITCODE."
    }
}

function Test-Administrator {
    return ([Security.Principal.WindowsPrincipal][Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole(
        [Security.Principal.WindowsBuiltInRole]::Administrator)
}

function Install-HostPackage {
    param([string]$Id, [string]$Version)

    $winget = Get-Command winget.exe -CommandType Application -ErrorAction SilentlyContinue
    if (-not $winget) {
        throw 'Install or update Windows App Installer to get WinGet, then rerun this script.'
    }
    $installArguments = @('install', '--id', $Id, '--exact', '--source', 'winget')
    if ($Version) { $installArguments += @('--version', $Version) }
    Invoke-Checked $winget.Source $installArguments
    $machinePath = [Environment]::GetEnvironmentVariable('Path', 'Machine')
    $userPath = [Environment]::GetEnvironmentVariable('Path', 'User')
    $env:Path = "$env:Path;$machinePath;$userPath"
}

function Install-ArduinoDependencies {
    param([string]$DataDirectory)

    $arduino = Get-Command arduino-cli.exe -CommandType Application -ErrorAction SilentlyContinue
    if (-not $arduino) {
        Install-HostPackage -Id 'ArduinoSA.CLI' -Version '1.5.1'
        $arduino = Get-Command arduino-cli.exe -CommandType Application -ErrorAction SilentlyContinue
    }
    if (-not $arduino) {
        throw 'Arduino CLI is not available yet. Reopen PowerShell and rerun the script.'
    }

    $tinyCircuitsIndex = 'https://files.tinycircuits.com/ArduinoBoards/package_tinycircuits_index.json'
    $config = Invoke-Checked $arduino.Source @('config', 'dump', '--format', 'json') |
        Out-String | ConvertFrom-Json
    if ($config.config.board_manager.additional_urls -notcontains $tinyCircuitsIndex) {
        Invoke-Checked $arduino.Source @('config', 'add', 'board_manager.additional_urls', $tinyCircuitsIndex)
    }
    Invoke-Checked $arduino.Source @('core', 'update-index')
    Invoke-Checked $arduino.Source @('core', 'install', 'arduino:samd@1.8.14', 'TinyCircuits:samd@1.1.0')
    Invoke-Checked $arduino.Source @('board', 'details', '--fqbn', 'TinyCircuits:samd:tinyscreen') | Out-Null
    Write-Host 'Verified the TinyScreen+ board definition.'
    Invoke-Checked $arduino.Source @('lib', 'update-index')
    Invoke-Checked $arduino.Source @('lib', 'install', 'TinyScreen@1.1.0', 'Keyboard@1.0.7')

    $libraries = Invoke-Checked $arduino.Source @('lib', 'list', '--format', 'json') |
        Out-String | ConvertFrom-Json
    foreach ($expected in @(@{ Name = 'TinyScreen'; Version = '1.1.0' }, @{ Name = 'Keyboard'; Version = '1.0.7' })) {
        $installed = $libraries.installed_libraries | Where-Object {
            $_.library.name -eq $expected.Name -and $_.library.version -eq $expected.Version
        }
        if (-not $installed) {
            throw "Arduino CLI did not find $($expected.Name) $($expected.Version) after installation."
        }
    }
    $stble = @($libraries.installed_libraries | Where-Object { $_.library.name -eq 'STBLE' })
    if ($stble.library.version -contains '0.1.0') {
        Write-Host 'STBLE 0.1.0 is already installed.'
        return
    }

    $sketchbook = (Invoke-Checked $arduino.Source @('config', 'get', 'directories.user') | Out-String).Trim()
    if (-not $sketchbook -or -not [IO.Path]::IsPathRooted($sketchbook)) {
        throw 'Arduino CLI did not report an absolute sketchbook path.'
    }
    $libraryDirectory = Join-Path $sketchbook 'libraries'
    if ($stble.Count -or (Test-Path -LiteralPath (Join-Path $libraryDirectory 'STBLE'))) {
        throw 'A different or incomplete STBLE installation exists. Review it before installing 0.1.0.'
    }
    $stbleUri = 'https://raw.githubusercontent.com/TinyCircuits/TinyCircuits-TinyShield-BLE-ASD2116/2dcc1d42c2e63ca9d489355665cf2769600d94d8/examples/STBLE.zip'
    $stbleHash = '87FDECA756B3627A174A9F6D92D678694E45CB36C8F4A0671C317FC408CBE7BF'
    $stbleArchive = Join-Path $DataDirectory 'STBLE-0.1.0.zip'
    if (-not (Test-Path -LiteralPath $stbleArchive -PathType Leaf)) {
        Invoke-WebRequest -Uri $stbleUri -UseBasicParsing -TimeoutSec 60 -OutFile $stbleArchive
    }
    if ((Get-FileHash -LiteralPath $stbleArchive -Algorithm SHA256).Hash -cne $stbleHash) {
        throw "STBLE archive checksum mismatch. Review '$stbleArchive' before rerunning."
    }
    New-Item -ItemType Directory -Path $libraryDirectory -Force | Out-Null
    Expand-Archive -LiteralPath $stbleArchive -DestinationPath $libraryDirectory
    $libraries = Invoke-Checked $arduino.Source @('lib', 'list', '--format', 'json') |
        Out-String | ConvertFrom-Json
    $installed = $libraries.installed_libraries | Where-Object {
        $_.library.name -eq 'STBLE' -and $_.library.version -eq '0.1.0'
    }
    if (-not $installed) {
        throw 'Arduino CLI did not find STBLE 0.1.0 after installation.'
    }
    Write-Host 'Installed and verified STBLE 0.1.0.'
}

function Find-Python311 {
    $launcher = Get-Command py.exe -CommandType Application -ErrorAction SilentlyContinue
    if ($launcher) {
        # An installed launcher may report that this particular Python version is missing.
        $ErrorActionPreference = 'Continue'
        $python = & $launcher.Source -3.11 -c 'import sys; print(sys.executable)' 2>$null
        if ($LASTEXITCODE -eq 0 -and $python -and (Test-Path -LiteralPath $python -PathType Leaf)) {
            return $python
        }
    }
}

function Get-HostIPv4 {
    param([string]$InterfaceAlias)

    if ($InterfaceAlias -and $InterfaceAlias -ne 'auto') {
        $address = Get-NetIPAddress -InterfaceAlias $InterfaceAlias -AddressFamily IPv4 -AddressState Preferred -ErrorAction SilentlyContinue |
            Select-Object -First 1
        if ($address) { return $address.IPAddress }
        Write-Warning "No preferred IPv4 on '$InterfaceAlias'. Falling back to the default-gateway interface."
    }

    $gateway = Get-NetIPConfiguration |
        Where-Object { $_.IPv4DefaultGateway -ne $null -and $_.NetAdapter.Status -eq 'Up' } |
        Select-Object -First 1
    if ($gateway) {
        $address = @($gateway.IPv4Address) | Select-Object -First 1
        if ($address) { return $address.IPAddress }
    }

    $address = Get-NetIPAddress -AddressFamily IPv4 -AddressState Preferred |
        Where-Object { $_.IPAddress -ne '127.0.0.1' } | Select-Object -First 1
    if ($address) { return $address.IPAddress }

    throw "Could not determine this host's LAN IPv4 address."
}

function Add-TraceHubFirewallRule {
    param([int]$ShellPort)

    $ruleName = "TraceHub rev shell $ShellPort"
    if (Get-NetFirewallRule -DisplayName $ruleName -ErrorAction SilentlyContinue) {
        Write-Host "Firewall rule '$ruleName' already exists."
        return
    }
    New-NetFirewallRule -DisplayName $ruleName -Direction Inbound -Protocol TCP -LocalPort $ShellPort -Action Allow | Out-Null
    Write-Host "Added firewall rule '$ruleName' for inbound TCP $ShellPort."
}

function Find-TinyScreenPort {
    param([string]$Arduino)

    $boards = Invoke-Checked $Arduino @('board', 'list', '--format', 'json') | Out-String | ConvertFrom-Json
    $serialPorts = @()
    foreach ($board in @($boards)) {
        $address = $board.port.address
        if (-not $address) { $address = $board.address }
        if (-not $address) { continue }
        $serialPorts += $address
        $matching = @($board.matching_boards | ForEach-Object { $_.fqbn })
        if ($matching -contains 'TinyCircuits:samd:tinyscreen') {
            return $address
        }
    }
    # The BLE shield can hide the board's identity from the bootloader.
    # If exactly one serial port is present, try it and let the operator confirm.
    if ($serialPorts.Count -eq 1) {
        Write-Warning "TinyScreen+ not identified by board list; trying the only serial port $($serialPorts[0])."
        return $serialPorts[0]
    }
    return $null
}

# ---- Main flow ----
$dataDirectory = Join-Path $env:LOCALAPPDATA 'TraceDeck'
New-Item -ItemType Directory -Path $dataDirectory -Force | Out-Null

if ($SkipArduino) {
    Write-Host 'Step 1 of 5: Arduino setup skipped.'
} else {
    Write-Host 'Step 1 of 5: Install Arduino CLI, board packages, and libraries.'
    Install-ArduinoDependencies -DataDirectory $dataDirectory
}

Write-Host 'Step 2 of 5: Set up Python and install TraceHub dependencies.'
$venv = Join-Path $dataDirectory 'venv'
$traceHubPython = Join-Path $venv 'Scripts\python.exe'
if (-not (Test-Path -LiteralPath $traceHubPython -PathType Leaf)) {
    $python = Find-Python311
    if (-not $python) {
        Install-HostPackage -Id 'Python.Python.3.11'
        $python = Find-Python311
    }
    if (-not $python) {
        throw 'Python 3.11 is unavailable. Reopen PowerShell and check that py -3.11 works.'
    }
    Invoke-Checked $python @('-m', 'venv', $venv)
}
Invoke-Checked $traceHubPython @('-m', 'pip', '--disable-pip-version-check', 'install', '-r', $requirements)

if ($SkipFirewall) {
    Write-Host 'Step 3 of 5: Firewall rule skipped.'
} elseif (-not (Test-Administrator)) {
    Write-Warning "Step 3 of 5: Skipped the firewall rule. Run PowerShell as Administrator to allow inbound TCP $ShellPort for the reverse shell."
} else {
    Write-Host "Step 3 of 5: Open inbound TCP $ShellPort for the reverse shell."
    Add-TraceHubFirewallRule -ShellPort $ShellPort
}

$hostIp = Get-HostIPv4 -InterfaceAlias $InterfaceAlias
Write-Host "Using LAN address $hostIp for the firmware and TraceHub."

if ($SkipFlash) {
    Write-Host 'Step 4 of 5: Firmware flash skipped.'
} else {
    Write-Host 'Step 4 of 5: Compile and flash the TinyScreen+ firmware.'
    Write-Host 'If the board is not detected, power it off, detach the BLE shield, double-tap reset, then rerun. Reattach the shield after flashing.'
    $arduino = Get-Command arduino-cli.exe -CommandType Application -ErrorAction SilentlyContinue
    if (-not $arduino) {
        throw 'Arduino CLI is not available. Run without -SkipArduino first, or reopen PowerShell.'
    }
    if ($Port) {
        $port = $Port
        Write-Host "Using the requested port $port."
    } else {
        $port = Find-TinyScreenPort -Arduino $arduino.Source
    }
    if (-not $port) {
        throw 'No TinyScreen+ detected. Flash with the BLE shield detached, double-tap reset, or pass -Port COMx, or use -SkipFlash to continue without flashing.'
    }
    Write-Host "Flashing $port with -DTRACEHUB_SERVER_IP=$hostIp -DATTACKER_IP=$hostIp."
    Invoke-Checked $arduino.Source @(
        'compile', '--fqbn', 'TinyCircuits:samd:tinyscreen',
        '--build-property', "compiler.cpp.extra_flags=-DTRACEHUB_SERVER_IP=$hostIp -DATTACKER_IP=$hostIp",
        '--upload', '--port', $port, $sketch
    )
}

if ($NoStart) {
    Write-Host 'Step 5 of 5: TraceHub startup skipped.'
} else {
    Write-Host 'Step 5 of 5: Start TraceHub (demo role) on the LAN.'
    $env:TRACEDECK_SHELL_HOST = $hostIp
    Write-Host "TraceHub HTTP: http://${hostIp}:$HttpPort  ·  Reverse shell: ${hostIp}:$ShellPort"
    Write-Host 'Press Ctrl+C to stop TraceHub.'
    Push-Location -LiteralPath $projectRoot
    try {
        & $traceHubPython -B -m flask --app "tracehub:create_app('demo')" --no-debug run --host 0.0.0.0 --port $HttpPort
    }
    finally {
        Pop-Location
    }
}
