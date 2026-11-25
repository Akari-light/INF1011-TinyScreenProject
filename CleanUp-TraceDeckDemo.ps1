#Requires -Version 5.1
<#
.SYNOPSIS
Interactive cleanup for the TraceDeck demo: choose Host or Victim.

.DESCRIPTION
Host removes the reverse-shell firewall rule and all TraceHub data (SQLite
database, loot archives, and walk trials), plus the toolchain unless
-KeepToolchain is set.

Victim removes the artifacts and settings the BadUSB payloads created:
the Desktop marker file, the cookie dumps, temporary ZIP workspaces, the
REV_SHELL environment variable, the PowerShell execution policy change, the
NuGet provider, the PSSQLite module, and the UAC "never notify" change.

Stop TraceHub, close any reverse shell, and unplug the board first.
Use -WhatIf to preview file deletions and removals.

.PARAMETER Role
Host or Victim. Omit to choose interactively.

.PARAMETER OriginalExecutionPolicy
Execution policy to restore on the victim. Default Restricted.

.PARAMETER KeepToolchain
Host: keep the Python venv and Arduino downloads under %LOCALAPPDATA%\TraceDeck.

.PARAMETER KeepFirewallRule
Host: keep the reverse-shell firewall rule.

.EXAMPLE
.\Clean-TraceDeckDemo.ps1

.EXAMPLE
.\Clean-TraceDeckDemo.ps1 -Role Victim -WhatIf

.EXAMPLE
.\Clean-TraceDeckDemo.ps1 -Role Host -KeepToolchain
#>
[CmdletBinding(SupportsShouldProcess, ConfirmImpact = 'Medium')]
param(
    [ValidateSet('Host', 'Victim')]
    [string]$Role,
    [ValidateSet('Restricted', 'RemoteSigned', 'AllSigned', 'Unrestricted', 'Bypass')]
    [string]$OriginalExecutionPolicy = 'Restricted',
    [switch]$KeepToolchain,
    [switch]$KeepFirewallRule,
    [string[]]$DesktopPath = @(
        [Environment]::GetFolderPath('Desktop'),
        (Join-Path $env:USERPROFILE 'Desktop')
    )
)

$ErrorActionPreference = 'Stop'

function Test-Administrator {
    return ([Security.Principal.WindowsPrincipal][Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole(
        [Security.Principal.WindowsBuiltInRole]::Administrator)
}

function Get-PlainDirectory {
    param([string]$Path)

    if (-not (Test-Path -LiteralPath $Path)) { return }
    $directory = Get-Item -LiteralPath $Path -Force
    if (-not $directory.PSIsContainer) { throw "Expected a directory: $Path" }
    for ($ancestor = $directory; $ancestor; $ancestor = $ancestor.Parent) {
        if ($ancestor.LinkType -in @('Junction', 'SymbolicLink')) {
            throw "Refusing a linked directory: $($ancestor.FullName)"
        }
    }
    return $directory
}

function Remove-Target {
    param([string]$Path, [string]$Description)

    if (-not (Test-Path -LiteralPath $Path)) { return }
    $item = Get-Item -LiteralPath $Path -Force
    if ($item.LinkType) { throw "Refusing a linked path: $Path" }
    if ($PSCmdlet.ShouldProcess($Path, $Description)) {
        Remove-Item -LiteralPath $Path -Recurse -Force
        Write-Host "Removed: $Path"
    }
}

function Remove-DesktopMarker {
    foreach ($candidate in ($DesktopPath | Where-Object { $_ } | Select-Object -Unique)) {
        $desktop = Get-PlainDirectory $candidate
        if (-not $desktop) { continue }
        $markerPath = Join-Path $desktop.FullName 'TinyCircuitDemo.txt'
        if (-not (Test-Path -LiteralPath $markerPath -PathType Leaf)) { continue }
        $marker = Get-Item -LiteralPath $markerPath -Force
        if ($marker.LinkType) { throw "Refusing a linked marker file: $markerPath" }
        if ($marker.Length -le 1024 -and [IO.File]::ReadAllText($markerPath).Trim() -ceq 'you have been pwned by TraceDeck') {
            Remove-Target $markerPath 'Delete the Drop & Upload marker file'
        } else {
            Write-Warning "Preserving a Desktop file with unexpected contents: $markerPath"
        }
    }
}

function Get-PythonExe {
    $venvPython = Join-Path $env:LOCALAPPDATA 'TraceDeck\venv\Scripts\python.exe'
    if (Test-Path -LiteralPath $venvPython -PathType Leaf) { return $venvPython }
    foreach ($name in @('python.exe', 'py.exe')) {
        $command = Get-Command $name -CommandType Application -ErrorAction SilentlyContinue
        if ($command) { return $command.Source }
    }
    return $null
}

function Clear-TraceHubDatabase {
    $database = Join-Path $env:LOCALAPPDATA 'TraceDeck\tracehub.sqlite3'
    if (-not (Test-Path -LiteralPath $database -PathType Leaf)) {
        Write-Host 'No TraceHub database file to wipe.'
        return
    }
    $python = Get-PythonExe
    if (-not $python) {
        Write-Warning 'No Python interpreter found to wipe the database rows.'
        return
    }
    if (-not $PSCmdlet.ShouldProcess($database, 'Clear all loot and walk-trial rows before deleting the file')) {
        return
    }

    # Prefer the local helper. Fall back to an inline script so a GitHub ZIP
    # that excludes .local/ still clears the rows before the file is removed.
    $wipeScript = Join-Path $PSScriptRoot '.local\research\wipe_loot.py'
    $temporary = $null
    if (-not (Test-Path -LiteralPath $wipeScript -PathType Leaf)) {
        $temporary = Join-Path $env:TEMP ('tracehub-wipe-' + [guid]::NewGuid().ToString('N') + '.py')
        $inline = @'
import os, sqlite3
from pathlib import Path
db = Path(os.environ['LOCALAPPDATA']) / 'TraceDeck' / 'tracehub.sqlite3'
if db.is_file():
    con = sqlite3.connect(db)
    try:
        names = {row[0] for row in con.execute("SELECT name FROM sqlite_master WHERE type = 'table'")}
        for table in ('loot', 'walk_trials'):
            if table in names:
                con.execute(f'DELETE FROM {table}')
        con.commit()
        con.execute('VACUUM')
    finally:
        con.close()
    print(f'Cleared {db}')
'@
        Set-Content -LiteralPath $temporary -Value $inline -Encoding UTF8
        $wipeScript = $temporary
    }
    try {
        & $python -B $wipeScript
        if ($LASTEXITCODE -ne 0) {
            throw "Wiping the database rows failed with exit code $LASTEXITCODE."
        }
    } finally {
        if ($temporary -and (Test-Path -LiteralPath $temporary)) {
            Remove-Item -LiteralPath $temporary -Force
        }
    }
}

function Clear-HostData {
    Write-Host ''
    Write-Host '=== Host cleanup: TraceHub data and firewall rule ==='

    $running = @(Get-CimInstance Win32_Process -Filter "Name LIKE 'python%.exe' OR Name = 'py.exe' OR Name = 'flask.exe'" |
        Where-Object { $_.CommandLine -match '(?i)\btracehub(?=[:.\s''"]|$)' })
    if ($running.Count) {
        throw "Stop TraceHub or walk capture first. Matching process IDs: $($running.ProcessId -join ', ')."
    }

    if (-not $KeepFirewallRule) {
        foreach ($ruleName in @('TraceHub rev shell 4444', 'TraceHub rev shell 8080', 'TraceHub upload 8080')) {
            $rules = @(Get-NetFirewallRule -PolicyStore PersistentStore -ErrorAction SilentlyContinue |
                Where-Object { $_.DisplayName -ceq $ruleName })
            foreach ($rule in $rules) {
                if ($PSCmdlet.ShouldProcess($rule.DisplayName, 'Remove demo firewall rule')) {
                    $rule | Remove-NetFirewallRule
                    Write-Host "Removed firewall rule: $($rule.DisplayName)"
                }
            }
        }
    }

    # The app hard-codes %LOCALAPPDATA%\TraceDeck\tracehub.sqlite3 in
    # tracehub/__init__.py (line 22) and tracehub/walk/trials.py (line 17).
    $hostData = Get-PlainDirectory (Join-Path $env:LOCALAPPDATA 'TraceDeck')
    if (-not $hostData) {
        Write-Warning "No TraceDeck data directory under $env:LOCALAPPDATA. If the setup ran under a different Windows user, rerun this script as that user."
        return
    }

    # Clear the rows first so the demo data is gone even if the file cannot be removed.
    Clear-TraceHubDatabase

    $dataNames = @(
        'tracehub.sqlite3', 'tracehub.sqlite3-wal', 'tracehub.sqlite3-shm', 'tracehub.sqlite3-journal',
        'helpers.json'
    )
    foreach ($name in $dataNames) {
        Remove-Target (Join-Path $hostData.FullName $name) 'Delete TraceHub data'
    }
    foreach ($sub in @('archives', 'walk-trials')) {
        $path = Join-Path $hostData.FullName $sub
        if (Test-Path -LiteralPath $path) {
            Remove-Target $path "Delete $sub (saved loot and trials)"
        }
    }

    if (-not $KeepToolchain) {
        Write-Host 'Removing the full TraceDeck toolchain and data directory.'
        Remove-Target $hostData.FullName 'Delete the TraceDeck data and toolchain directory'
    } else {
        Write-Host 'Kept the Python venv and Arduino downloads (-KeepToolchain).'
    }

    # Verify nothing was left behind (a locked file or wrong user profile is the usual cause).
    if (-not $WhatIfPreference) {
        $leftovers = @()
        foreach ($name in $dataNames) {
            $path = Join-Path $hostData.FullName $name
            if (Test-Path -LiteralPath $path) { $leftovers += $path }
        }
        foreach ($sub in @('archives', 'walk-trials')) {
            $path = Join-Path $hostData.FullName $sub
            if (Test-Path -LiteralPath $path) { $leftovers += $path }
        }
        if (-not $KeepToolchain -and (Test-Path -LiteralPath $hostData.FullName)) {
            $leftovers += $hostData.FullName
        }
        if ($leftovers.Count) {
            throw "TraceHub data is still present: $($leftovers -join ', '). Stop TraceHub, confirm the file is not locked, and rerun."
        }
        Write-Host 'Verified: no database, archives, walk trials, or helpers file remain.'
    }
}

function Clear-VictimArtifacts {
    Write-Host ''
    Write-Host '=== Victim cleanup: BadUSB payload artifacts and settings ==='

    Remove-DesktopMarker

    $temp = Get-PlainDirectory $env:TEMP
    if ($temp) {
        foreach ($name in @('edge_cookie_dump.txt', 'chrome_cookies_dump.txt')) {
            Remove-Target (Join-Path $temp.FullName $name) 'Delete the cookie dump'
        }
        foreach ($directory in (Get-ChildItem -LiteralPath $temp.FullName -Directory -Force)) {
            if ($directory.Name -match '^TraceDeck-[0-9a-f]{8}(?:-[0-9a-f]{4}){3}-[0-9a-f]{12}$') {
                Remove-Target $directory.FullName 'Delete a Drop & Upload temporary workspace'
            }
        }
    }

    # Pwning leaves the reverse-shell body in the REV_SHELL user variable.
    $revShell = [Environment]::GetEnvironmentVariable('REV_SHELL', 'User')
    if ($revShell) {
        if ($PSCmdlet.ShouldProcess('User environment variable REV_SHELL', 'Delete the reverse-shell variable')) {
            [Environment]::SetEnvironmentVariable('REV_SHELL', $null, 'User')
            Write-Host 'Removed the REV_SHELL user environment variable.'
        }
    }

    # The cookie jars set the LocalMachine policy to Unrestricted.
    $currentPolicy = Get-ExecutionPolicy -Scope LocalMachine
    if ($currentPolicy -ne $OriginalExecutionPolicy) {
        if (Test-Administrator) {
            if ($PSCmdlet.ShouldProcess('PowerShell execution policy', "Restore LocalMachine to $OriginalExecutionPolicy")) {
                Set-ExecutionPolicy -Scope LocalMachine -ExecutionPolicy $OriginalExecutionPolicy -Force
                Write-Host "Restored the LocalMachine execution policy to $OriginalExecutionPolicy."
            }
        } else {
            Write-Warning "Execution policy is $currentPolicy. Run as Administrator to restore $OriginalExecutionPolicy."
        }
    }

    # The cookie jars install the NuGet provider and PSSQLite.
    foreach ($nugetPath in @(
            (Join-Path $env:ProgramFiles 'PackageManagement\ProviderAssemblies\NuGet'),
            (Join-Path $env:LOCALAPPDATA 'PackageManagement\ProviderAssemblies\NuGet'))) {
        Remove-Target $nugetPath 'Remove the NuGet package provider'
    }
    if (Get-Module -ListAvailable -Name PSSQLite) {
        if ($PSCmdlet.ShouldProcess('PSSQLite', 'Uninstall the PowerShell module')) {
            Uninstall-Module -Name PSSQLite -Force -AllVersions -ErrorAction SilentlyContinue
            Write-Host 'Uninstalled the PSSQLite module.'
        }
    }
    foreach ($modulePath in @(
            (Join-Path $env:USERPROFILE 'Documents\WindowsPowerShell\Modules\PSSQLite'),
            (Join-Path $env:USERPROFILE 'Documents\PowerShell\Modules\PSSQLite'))) {
        Remove-Target $modulePath 'Remove a leftover PSSQLite module folder'
    }

    # De-Shield sets UAC to "never notify"; restore the default notify level.
    $policy = 'HKLM:\SOFTWARE\Microsoft\Windows\CurrentVersion\Policies\System'
    $enableLua = (Get-ItemProperty -Path $policy -Name 'EnableLUA' -ErrorAction SilentlyContinue).EnableLUA
    $promptAdmin = (Get-ItemProperty -Path $policy -Name 'ConsentPromptBehaviorAdmin' -ErrorAction SilentlyContinue).ConsentPromptBehaviorAdmin
    if ($enableLua -eq 1 -and $promptAdmin -ne 0) {
        Write-Host 'UAC is already enabled (nothing to restore).'
    } elseif (-not (Test-Administrator)) {
        Write-Warning 'UAC is set to never notify. Run as Administrator to restore it, or open the UAC slider and pick the default level.'
    } elseif ($PSCmdlet.ShouldProcess('Windows UAC', 'Restore the default notify level')) {
        Set-ItemProperty -Path $policy -Name 'EnableLUA' -Value 1 -Type DWord
        Set-ItemProperty -Path $policy -Name 'ConsentPromptBehaviorAdmin' -Value 5 -Type DWord
        Set-ItemProperty -Path $policy -Name 'PromptOnSecureDesktop' -Value 1 -Type DWord
        Write-Host 'Restored UAC to the default notify level. Restart Windows for it to take effect.'
    }
}

if (-not $Role) {
    Write-Host ''
    Write-Host 'TraceDeck demo cleanup'
    Write-Host '  1. Host   - TraceHub server: firewall rule, SQLite database, loot archives, walk trials'
    Write-Host '  2. Victim - machine that ran the BadUSB payloads'
    while (-not $Role) {
        $answer = Read-Host 'Choose 1 (host) or 2 (victim)'
        switch ($answer.Trim()) {
            '1' { $Role = 'Host' }
            '2' { $Role = 'Victim' }
            default { Write-Warning 'Enter 1 or 2.' }
        }
    }
}

Write-Host "Cleaning up as: $Role"

switch ($Role) {
    'Host' { Clear-HostData }
    'Victim' { Clear-VictimArtifacts }
}

Write-Host ''
Write-Host 'Cleanup finished.'
if (-not $WhatIfPreference) {
    Write-Warning 'Review Windows Security, UAC, the execution policy, and any open browser windows. This script does not uninstall shared tools such as Arduino CLI or Python.'
}
