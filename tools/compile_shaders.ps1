# Compile Nine Under shaders to SPIR-V.
# Prefers slangc when available; falls back to glslangValidator (Vulkan SDK).

$ErrorActionPreference = "Stop"
$Shaders = Join-Path $PSScriptRoot "..\shaders"
Set-Location $Shaders

function Find-Tool($name) {
    $cmd = Get-Command $name -ErrorAction SilentlyContinue
    if ($cmd) { return $cmd.Source }
    if ($env:VULKAN_SDK) {
        $candidate = Join-Path $env:VULKAN_SDK "Bin\$name.exe"
        if (Test-Path $candidate) { return $candidate }
    }
    return $null
}

$slangc = Find-Tool "slangc"
$glslang = Find-Tool "glslangValidator"
if (-not $glslang) { $glslang = Find-Tool "glslang" }

if ($slangc) {
    Write-Host "Using slangc: $slangc"
    & $slangc mesh.slang -target spirv -stage vertex -entry vertexMain -o mesh.vert.spv
    & $slangc mesh.slang -target spirv -stage fragment -entry fragmentMain -o mesh.frag.spv
} elseif ($glslang) {
    Write-Host "Using glslang: $glslang"
    & $glslang -V -S vert mesh.vert -o mesh.vert.spv
    & $glslang -V -S frag mesh.frag -o mesh.frag.spv
} else {
    Write-Error "Neither slangc nor glslang/glslangValidator found. Install Vulkan SDK or Slang."
    exit 1
}

Write-Host "Shaders compiled to shaders/*.spv"
