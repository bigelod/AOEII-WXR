# AOEII-WXR - deploy the proxy to a game directory.
#
# Two guards, both from Docs/general_wxr/TOOLS.md:
#
#   1. Freshness. Refuses to deploy a binary older than the newest source
#      file. Hours have been lost on headsets running a stale DLL while the
#      source on the desk said otherwise.
#   2. SHA-256 match. Copying is not verification. The script prints
#      match=True only when the bytes at the destination equal the bytes
#      built, so "it deployed" is a measurement and not a hope.
#
# Usage:
#   build\deploy.ps1 -GameDir "E:\tmp\Q3Mods\Age of Empires II\age2_x1"

param(
  [Parameter(Mandatory = $true)][string]$GameDir,
  [switch]$Verbose_   # also drop the verbose flag file
)

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path)
$dll  = Join-Path $root 'build\out\d3d9.dll'

if (-not (Test-Path $dll)) {
  Write-Host "ERROR: $dll does not exist. Run build\build.bat first." -ForegroundColor Red
  exit 1
}
if (-not (Test-Path $GameDir)) {
  Write-Host "ERROR: game directory not found: $GameDir" -ForegroundColor Red
  exit 1
}

# --- guard 1: freshness
$built = (Get-Item $dll).LastWriteTimeUtc
$newestSrc = Get-ChildItem (Join-Path $root 'src') -File |
             Sort-Object LastWriteTimeUtc -Descending | Select-Object -First 1
if ($newestSrc.LastWriteTimeUtc -gt $built) {
  Write-Host "ERROR: the binary is older than the source." -ForegroundColor Red
  Write-Host "  d3d9.dll        $built"
  Write-Host "  $($newestSrc.Name)  $($newestSrc.LastWriteTimeUtc)"
  Write-Host "Rebuild before deploying. Refusing to ship a stale DLL." -ForegroundColor Red
  exit 1
}

# --- the exe must be where we think it is
$exes = @('age2_x1.exe', 'empires2.exe') |
        Where-Object { Test-Path (Join-Path $GameDir $_) }
if ($exes.Count -eq 0) {
  Write-Host "WARNING: no age2_x1.exe or empires2.exe in $GameDir." -ForegroundColor Yellow
  Write-Host "         A DLL is normally loaded from the directory holding the EXE,"
  Write-Host "         so this may be the wrong folder. Deploying to BOTH the game"
  Write-Host "         root and age2_x1\ is the safe move - the log records which"
  Write-Host "         copy actually loaded."
}

# --- copy
$dest = Join-Path $GameDir 'd3d9.dll'
Copy-Item $dll $dest -Force

if ($Verbose_) {
  Set-Content (Join-Path $GameDir 'aoe2wxr_verbose.txt') '' -Encoding utf8
  Write-Host "verbose logging: ON"
}

# --- guard 2: verify the bytes
$a = (Get-FileHash $dll  -Algorithm SHA256).Hash
$b = (Get-FileHash $dest -Algorithm SHA256).Hash
Write-Host ""
Write-Host "source  $dll"
Write-Host "dest    $dest"
Write-Host "sha256  $a"
Write-Host "match=$($a -eq $b)"
if ($a -ne $b) {
  Write-Host "ERROR: the destination does not match what was built." -ForegroundColor Red
  exit 1
}

# --- the architecture of the real D3D9. Taking x64 out of a DXVK archive is
# the easy mistake, and a 64-bit DLL in a 32-bit process simply does not load:
# no crash, no message, just a renderer that never appears.
function Get-PeMachine([string]$file) {
  $fs = [System.IO.File]::OpenRead($file)
  try {
    $b = New-Object byte[] 4096
    $n = $fs.Read($b, 0, 4096)
    if ($n -lt 64) { return 0 }
    $pe = [BitConverter]::ToInt32($b, 0x3c)
    if ($pe -lt 0 -or $pe + 6 -gt $n) { return 0 }
    return [BitConverter]::ToUInt16($b, $pe + 4)
  } finally { $fs.Close() }
}

$real = Join-Path $GameDir 'd3d9_real.dll'
if (Test-Path $real) {
  $m = Get-PeMachine $real
  if ($m -eq 0x8664) {
    Write-Host ""
    Write-Host "ERROR: d3d9_real.dll is 64-bit (machine 0x8664)." -ForegroundColor Red
    Write-Host "       The game is a 32-bit process. You need the 32-BIT d3d9.dll:"
    Write-Host "         upstream .tar.gz layout : x32\d3d9.dll   (not x64\)"
    Write-Host "         Wine / Proton layout    : syswow64\d3d9.dll  (not system32\)"
    Write-Host "       System32 holds the 64-bit files - the naming is backwards."
    exit 1
  } elseif ($m -ne 0x014c) {
    Write-Host "WARNING: d3d9_real.dll machine is 0x$('{0:X4}' -f $m), expected 0x014C." -ForegroundColor Yellow
  } else {
    # Identify it honestly. DXVK's version RESOURCE impersonates a Windows
    # build number - FileVersion reads '10.0.17763.1 (WinBuild...)' - so it
    # looks like a system DLL at a glance and cannot be used to check the pin.
    # Two things do identify it: ProductName, and a bare version string DXVK
    # embeds in the image ('v1.10.3').
    $vi = (Get-Item $real).VersionInfo
    $bytes = [System.IO.File]::ReadAllBytes($real)
    $ascii = [System.Text.Encoding]::ASCII.GetString($bytes)
    $isDxvk = ($vi.ProductName -eq 'DXVK')
    $hasPin = $ascii.Contains('v1.10.3')

    if ($isDxvk -and $hasPin) {
      Write-Host "d3d9_real.dll: DXVK 1.10.3, 32-bit - matches the pin"
    } elseif ($isDxvk) {
      Write-Host "d3d9_real.dll: DXVK, 32-bit, but NOT 1.10.3 (the pin)." -ForegroundColor Yellow
    } else {
      Write-Host "d3d9_real.dll: 32-bit, $($vi.CompanyName) $($vi.ProductVersion)"
      Write-Host "  NOTE: not DXVK. The pin is DXVK 1.10.3; this is fine for a" -ForegroundColor Yellow
      Write-Host "        desktop smoke test but will not work under Wine." -ForegroundColor Yellow
    }
  }
}

# --- report the rest of the stack, since our DLL alone does nothing
Write-Host ""
Write-Host "stack check in $GameDir"
foreach ($n in @('ddraw.dll', 'd3d9_real.dll')) {
  $p = Join-Path $GameDir $n
  if (Test-Path $p) { Write-Host "  [ok]      $n" }
  else              { Write-Host "  [missing] $n" -ForegroundColor Yellow }
}

# Either config name satisfies cnc-ddraw, and ddraw.ini wins when both exist.
# Listing both as required produced a false 'missing' - the exact pattern that
# already cost one wrong diagnosis in this project.
$cfg = @('ddraw.ini', 'cnc-ddraw.ini') |
       Where-Object { Test-Path (Join-Path $GameDir $_) }
if ($cfg) {
  Write-Host "  [ok]      config: $($cfg -join ', ')  (ddraw.ini wins if both)"
} else {
  Write-Host "  [missing] no ddraw.ini or cnc-ddraw.ini" -ForegroundColor Yellow
}

if (Test-Path (Join-Path $GameDir 'ddraw.dll')) {
  $dv = (Get-Item (Join-Path $GameDir 'ddraw.dll')).VersionInfo.FileVersion
  Write-Host "  cnc-ddraw $dv"
  if ($dv -match '^1\.1\.4') {
    Write-Host "  ERROR: the 1.1.4.x lineage has NO Direct3D 9 renderer." -ForegroundColor Red
    Write-Host "         renderer=direct3d9 is ignored silently and we never load." -ForegroundColor Red
  }
}
Write-Host ""
Write-Host "After the run, read $GameDir\aoe2wxr.log"
