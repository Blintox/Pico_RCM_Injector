param(
    [string]$Board = "pico",
    [string]$Generator = "Ninja",
    [string]$SdkPath = $env:PICO_SDK_PATH
)

$ErrorActionPreference = "Stop"

function Add-PathIfExists {
    param([string]$Path)

    if ($Path -and (Test-Path -LiteralPath $Path) -and (($env:Path -split ';') -notcontains $Path)) {
        $env:Path = "$Path;$env:Path"
    }
}

Add-PathIfExists "C:\Program Files\CMake\bin"
Add-PathIfExists "C:\Program Files\Git\cmd"
Add-PathIfExists "C:\Program Files (x86)\Arm GNU Toolchain arm-none-eabi\14.2 rel1\bin"
Add-PathIfExists (Join-Path $env:LOCALAPPDATA "Microsoft\WinGet\Packages\Ninja-build.Ninja_Microsoft.Winget.Source_8wekyb3d8bbwe")

function Require-Command {
    param([string]$Name)

    if (-not (Get-Command $Name -ErrorAction SilentlyContinue)) {
        throw "$Name was not found on PATH. Install the Pico SDK toolchain or open a terminal where the Pico tools are configured."
    }
}

Require-Command cmake
Require-Command arm-none-eabi-gcc

if ($Generator -eq "Ninja") {
    Require-Command ninja
}

if (-not $SdkPath) {
    throw "PICO_SDK_PATH is not set. Pass -SdkPath C:\path\to\pico-sdk or set the PICO_SDK_PATH environment variable."
}

$resolvedSdkPath = (Resolve-Path -LiteralPath $SdkPath).Path
$importFile = Join-Path $resolvedSdkPath "external\pico_sdk_import.cmake"
if (-not (Test-Path -LiteralPath $importFile)) {
    throw "PICO_SDK_PATH does not look like a Pico SDK checkout: $resolvedSdkPath"
}

$env:PICO_SDK_PATH = $resolvedSdkPath

cmake -S . -B build -G $Generator "-DPICO_BOARD=$Board"
cmake --build build

$uf2 = Join-Path (Resolve-Path -LiteralPath build).Path "pico_rcm_injector.uf2"
if (Test-Path -LiteralPath $uf2) {
    Write-Host "UF2 ready: $uf2"
} else {
    throw "Build completed, but pico_rcm_injector.uf2 was not found."
}
