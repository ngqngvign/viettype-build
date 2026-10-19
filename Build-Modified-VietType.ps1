# Copyright 2026; SPDX-License-Identifier: GPL-3.0-only
# No COM registration, no admin, no installation, no silent MSI execution.
[CmdletBinding()]
param(
    [ValidateSet('x64','Win32')][string]$Platform = 'x64',
    [switch]$BuildSetup,
    [switch]$UseExistingCheckout
)
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $MyInvocation.MyCommand.Path
$src = Join-Path $root 'VietType-upstream'
$logs = Join-Path $root 'logs'
New-Item -ItemType Directory -Path $logs -Force | Out-Null
function Require($command) {
    if (-not (Get-Command $command -ErrorAction SilentlyContinue)) {
        throw "Missing dependency: $command"
    }
}
Require 'git'
$python = Get-Command 'py' -ErrorAction SilentlyContinue
if (-not $python) { $python = Get-Command 'python' -ErrorAction SilentlyContinue }
if (-not $python) { throw 'Python 3 required for the guarded patch. Install Python 3.' }
$pythonArg = @()
if ($python.Name -match '^py(\.exe)?$') { $pythonArg = @('-3') }
$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
if (-not (Test-Path $vswhere)) {
    throw 'VS 2022/2026 C++ Build Tools (MSBuild + Windows SDK + ATL) not found.'
}
$vsdir = & $vswhere -latest -products '*' -requires Microsoft.Component.MSBuild -property installationPath
if (-not $vsdir) { throw 'MSBuild installation not found' }
$msbuild = Join-Path ($vsdir | Select-Object -First 1) 'MSBuild\Current\Bin\MSBuild.exe'
if (-not (Test-Path $msbuild)) { throw "MSBuild not found: $msbuild" }

if ($UseExistingCheckout) {
    if (-not (Test-Path (Join-Path $src '.git'))) {
        throw "No existing VietType checkout at $src"
    }
} else {
    if (Test-Path $src) { throw "Refusing to overwrite $src. Use -UseExistingCheckout explicitly." }
    Write-Host 'Cloning archived official VietType source...'
    & git clone --depth 1 --branch master 'https://github.com/dinhngtu/VietType.git' $src
    if ($LASTEXITCODE -ne 0) { throw 'git clone failed; no files modified or installed.' }
}
$commit = & git -C $src rev-parse HEAD
if ($LASTEXITCODE -ne 0) { throw 'Cannot determine upstream commit' }
$commit | Set-Content -Path (Join-Path $logs 'UPSTREAM_COMMIT.txt') -Encoding Ascii
Write-Host "Upstream commit: $commit"

# Test and apply guarded patch. Fails closed if upstream changed.
& $python.Source @pythonArg (Join-Path $root 'tools\apply_patch.py') --repo $src --dry-run
if ($LASTEXITCODE -ne 0) { throw 'Patch validation failed; stopped before changing upstream.' }
& $python.Source @pythonArg (Join-Path $root 'tools\apply_patch.py') --repo $src
if ($LASTEXITCODE -ne 0) { throw 'Patch application failed; build not attempted.' }

Push-Location $src
try {
    # Upstream CI builds both Win32 and x64 DLLs before producing an MSI.
    $architectures = @($Platform)
    if ($BuildSetup) { $architectures = @('Win32', 'x64') }
    foreach ($arch in $architectures) {
        $log = Join-Path $logs "build-Release-$arch.log"
        & $msbuild '-m' '-target:Restore' '-p:Configuration=Release' "-p:Platform=$arch" '.' *> (Join-Path $logs "restore-Release-$arch.log")
        if ($LASTEXITCODE -ne 0) { throw "NuGet restore FAILED for $arch (see logs)." }
        & $msbuild '-m' '-target:VietTypeATL:Rebuild' '-p:Configuration=Release' "-p:Platform=$arch" '.' *> $log
        if ($LASTEXITCODE -ne 0) { throw "TSF DLL build FAILED for $arch. See $log" }
        Write-Host "TSF DLL build SUCCEEDED ($arch). Log: $log"
        $out = Join-Path $root "output\$arch"
        New-Item -ItemType Directory -Force -Path $out | Out-Null
        $dll = Get-ChildItem -Path (Join-Path $src "$arch\Release") -Filter 'VietTypeATL.dll' -File -Recurse -ErrorAction SilentlyContinue | Select-Object -First 1
        if (-not $dll) { throw "Build reported success but output DLL was not found for $arch." }
        Copy-Item $dll.FullName (Join-Path $out 'VietTypeATL.dll') -Force
        Get-FileHash (Join-Path $out 'VietTypeATL.dll') -Algorithm SHA256 | Format-List | Out-String | Set-Content (Join-Path $out 'SHA256.txt')
        Write-Host "DLL and SHA256: $out"
    }
    $out = Join-Path $root "output\$Platform"
    if ($BuildSetup) {
        $setupLog = Join-Path $logs "setup-Release-$Platform.log"
        & $msbuild '-m' '-target:VietTypeSetup:Rebuild' '-p:Configuration=Release' "-p:Platform=$Platform" '.' *> $setupLog
        if ($LASTEXITCODE -ne 0) { throw "MSI build FAILED. See $setupLog" }
        $msis = Get-ChildItem (Join-Path $src 'VietTypeSetup\bin') -Filter '*.msi' -File -Recurse -ErrorAction SilentlyContinue
        if (-not $msis) { throw 'MSI build reported success but MSI was not found.' }
        foreach ($msi in $msis) {
            Copy-Item $msi.FullName (Join-Path $out $msi.Name) -Force
        }
        Write-Warning 'MSI is NOT tested on a real Windows VM. Do not install on your daily-use machine before uninstall testing.'
    }
} finally {
    Pop-Location
}
Write-Host 'Finished. Nothing was installed, registered, or executed from the build output.'
