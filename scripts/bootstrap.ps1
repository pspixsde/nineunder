# One-time / refresh bootstrap for Nine Under on Windows.
# Sets up vcpkg, checks Ninja / MSVC / Vulkan SDK, and prints the next commands.

$ErrorActionPreference = "Stop"
$Root = Split-Path -Parent $PSScriptRoot

Write-Host "== Nine Under bootstrap ==" -ForegroundColor Cyan

# Git LFS
git -C $Root lfs install 2>$null
Write-Host "[ok] Git LFS"

# vcpkg
$defaultVcpkg = "C:\dev\vcpkg"
if (-not $env:VCPKG_ROOT -or -not (Test-Path "$env:VCPKG_ROOT\scripts\buildsystems\vcpkg.cmake")) {
    if (-not (Test-Path "$defaultVcpkg\scripts\buildsystems\vcpkg.cmake")) {
        Write-Host "Cloning vcpkg to $defaultVcpkg ..."
        New-Item -ItemType Directory -Force -Path (Split-Path $defaultVcpkg) | Out-Null
        git clone --depth 1 https://github.com/microsoft/vcpkg.git $defaultVcpkg
        & "$defaultVcpkg\bootstrap-vcpkg.bat" -disableMetrics
    }
    $env:VCPKG_ROOT = $defaultVcpkg
    Write-Host "[ok] VCPKG_ROOT=$env:VCPKG_ROOT (this session)"
    Write-Host "     Add permanently: [System] Environment Variables -> VCPKG_ROOT=$defaultVcpkg" -ForegroundColor Yellow
} else {
    Write-Host "[ok] VCPKG_ROOT=$env:VCPKG_ROOT"
}

# Ninja
$ninja = Get-Command ninja -ErrorAction SilentlyContinue
if (-not $ninja) {
    $candidates = @(
        "$env:LOCALAPPDATA\Programs\ninja\ninja.exe",
        "$env:LOCALAPPDATA\Microsoft\WinGet\Packages\Ninja-build.Ninja_Microsoft.Winget.Source_8wekyb3d8bbwe\ninja.exe"
    )
    foreach ($c in $candidates) {
        if (Test-Path $c) {
            $env:Path = "$(Split-Path $c);$env:Path"
            Write-Host "[ok] Added Ninja to PATH for this session: $c"
            break
        }
    }
    $ninja = Get-Command ninja -ErrorAction SilentlyContinue
}
if ($ninja) {
    Write-Host "[ok] Ninja: $($ninja.Source)"
} else {
    Write-Host "[missing] Ninja — install with: winget install Ninja-build.Ninja" -ForegroundColor Yellow
}

# MSVC
$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
$vsPath = $null
if (Test-Path $vswhere) {
    $vsPath = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath 2>$null
}
if ($vsPath) {
    Write-Host "[ok] MSVC tools: $vsPath"
} else {
    Write-Host "[missing] Visual Studio 2022 C++ tools (required for x64-windows vcpkg triplet)" -ForegroundColor Yellow
    Write-Host "  Install Build Tools:" -ForegroundColor Yellow
    Write-Host '  winget install Microsoft.VisualStudio.2022.BuildTools --override "--wait --passive --add Microsoft.VisualStudio.Workload.VCTools --includeRecommended"'
    Write-Host "  Then open 'x64 Native Tools Command Prompt for VS 2022' (or Developer PowerShell) before cmake."
}

# Vulkan
if (-not $env:VULKAN_SDK) {
    Write-Host "[missing] VULKAN_SDK — install: winget install KhronosGroup.VulkanSDK" -ForegroundColor Yellow
} else {
    Write-Host "[ok] VULKAN_SDK=$env:VULKAN_SDK"
}

Write-Host ""
Write-Host "Configure & build (from a VS x64 developer shell):" -ForegroundColor Cyan
Write-Host '  $env:VCPKG_ROOT = "C:\dev\vcpkg"   # if not permanent'
Write-Host "  cd `"$Root`""
Write-Host "  cmake --preset dev"
Write-Host "  cmake --build --preset dev"
Write-Host "  .\build\dev\game\NineUnder.exe"
