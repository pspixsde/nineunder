# One-time / refresh bootstrap for Nine Under on Windows.
# - Ensures Git LFS
# - Optionally clones vcpkg if VCPKG_ROOT is unset
# - Reminds about Vulkan SDK

$ErrorActionPreference = "Stop"
$Root = Split-Path -Parent $PSScriptRoot

Write-Host "== Nine Under bootstrap ==" -ForegroundColor Cyan

# Git LFS
git -C $Root lfs install 2>$null
Write-Host "Git LFS: ok"

if (-not $env:VCPKG_ROOT -or -not (Test-Path "$env:VCPKG_ROOT\scripts\buildsystems\vcpkg.cmake")) {
    $default = "C:\dev\vcpkg"
    if (-not (Test-Path $default)) {
        Write-Host "Cloning vcpkg to $default ..."
        New-Item -ItemType Directory -Force -Path (Split-Path $default) | Out-Null
        git clone https://github.com/microsoft/vcpkg.git $default
        & "$default\bootstrap-vcpkg.bat"
    }
    $env:VCPKG_ROOT = $default
    Write-Host "Set VCPKG_ROOT=$env:VCPKG_ROOT for this session."
    Write-Host "Add it permanently via System Environment Variables."
} else {
    Write-Host "VCPKG_ROOT=$env:VCPKG_ROOT"
}

if (-not $env:VULKAN_SDK) {
    Write-Host "WARNING: VULKAN_SDK is not set. Install from https://vulkan.lunarg.com/ or:" -ForegroundColor Yellow
    Write-Host "  winget install KhronosGroup.VulkanSDK"
} else {
    Write-Host "VULKAN_SDK=$env:VULKAN_SDK"
}

Write-Host ""
Write-Host "Configure & build:" -ForegroundColor Cyan
Write-Host "  cmake --preset dev"
Write-Host "  cmake --build --preset dev"
Write-Host "  .\build\dev\game\NineUnder.exe"
