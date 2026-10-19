# SPDX-License-Identifier: GPL-3.0-only
# Manage only HKCU\Software\VietType\SmartSwitch, not Windows input/TSF registry keys.
[CmdletBinding(DefaultParameterSetName='App')]
param(
    [Parameter(ParameterSetName='App', Mandatory=$true)]
    [ValidatePattern('^[^\\/]{1,180}$')][string]$ExeName,
    [Parameter(ParameterSetName='App', Mandatory=$true)]
    [ValidateSet('Vietnamese','English','Forget','Show')][string]$Mode,
    [Parameter(ParameterSetName='Global', Mandatory=$true)]
    [ValidateSet('Enable','Disable','Show')][string]$Memory
)
$ErrorActionPreference = 'Stop'
$root = 'HKCU:\Software\VietType\SmartSwitch'
if ($PSCmdlet.ParameterSetName -eq 'Global') {
    if ($Memory -eq 'Show') {
        $value = (Get-ItemProperty -Path $root -Name MemoryEnabled -ErrorAction SilentlyContinue).MemoryEnabled
        if ($null -eq $value) { $value = 1 }
        Write-Output "Per-app memory enabled: $([bool]$value)"
        return
    }
    New-Item -Path $root -Force | Out-Null
    $value = if ($Memory -eq 'Enable') { 1 } else { 0 }
    New-ItemProperty -Path $root -Name MemoryEnabled -PropertyType DWord -Value $value -Force | Out-Null
    Write-Output "MemoryEnabled=$value; no TSR processes or services changed."
    return
}
# Match Windows CharLowerBuffW for ordinary ASCII executable basenames.
$name = $ExeName.ToLowerInvariant()
$path = Join-Path $root $name
if ($Mode -eq 'Show') {
    $v = (Get-ItemProperty -Path $path -Name Mode -ErrorAction SilentlyContinue).Mode
    if ($null -eq $v) { Write-Output "${name}: inherit VietType's default" }
    else { Write-Output ("{0}: {1}" -f $name, $(if ($v -eq 1) {'Vietnamese'} else {'English'})) }
    return
}
if ($Mode -eq 'Forget') {
    if (Test-Path $path) { Remove-ItemProperty -Path $path -Name Mode -ErrorAction SilentlyContinue }
    Write-Output "$name preference removed (if present)"
    return
}
New-Item -Path $path -Force | Out-Null
$value = if ($Mode -eq 'Vietnamese') { 1 } else { 0 }
New-ItemProperty -Path $path -Name Mode -PropertyType DWord -Value $value -Force | Out-Null
Write-Output "Saved $name = $Mode; the TSF instance reads it when it next receives focus."
