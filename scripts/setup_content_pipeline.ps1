# One-time / refresh setup for the Nine Under 3D content pipeline (Windows).
# - Locates Blender 4.2 and sets NU_BLENDER (User env)
# - Sets NU_COMFYUI_URL (User env)
# - Installs the nu-content Python CLI
# - Smoke-tests Blender headless + sample prop

$ErrorActionPreference = "Stop"
$Root = Split-Path -Parent $PSScriptRoot

Write-Host "== Nine Under content pipeline setup ==" -ForegroundColor Cyan
Write-Host "Repo: $Root"

function Set-UserEnv([string]$Name, [string]$Value) {
    [System.Environment]::SetEnvironmentVariable($Name, $Value, "User")
    Set-Item -Path "Env:$Name" -Value $Value
    Write-Host "[ok] User env $Name=$Value" -ForegroundColor Green
}

# ---------------------------------------------------------------------------
# Blender
# ---------------------------------------------------------------------------
$blender = $null
if ($env:NU_BLENDER -and (Test-Path $env:NU_BLENDER)) {
    $blender = $env:NU_BLENDER
}
if (-not $blender) {
    $candidates = @(
        "C:\Program Files\Blender Foundation\Blender 4.2\blender.exe",
        "C:\Program Files\Blender Foundation\Blender 4.3\blender.exe",
        "C:\Program Files\Blender Foundation\Blender 4.1\blender.exe"
    )
    foreach ($c in $candidates) {
        if (Test-Path $c) { $blender = $c; break }
    }
}
if (-not $blender) {
    $cmd = Get-Command blender -ErrorAction SilentlyContinue
    if ($cmd) { $blender = $cmd.Source }
}

if (-not $blender) {
    Write-Host "[!!] Blender not found. Install Blender LTS 4.2, then re-run." -ForegroundColor Red
    Write-Host "     https://www.blender.org/download/lts/4-2/"
} else {
    Set-UserEnv "NU_BLENDER" $blender
    Write-Host "     Checking Blender --version ..."
    # Blender may print TBB warnings on stderr; do not treat as failure.
    $prevEap = $ErrorActionPreference
    $ErrorActionPreference = "Continue"
    & $blender --version 2>$null | Select-Object -First 2
    $ErrorActionPreference = $prevEap
}

# ---------------------------------------------------------------------------
# ComfyUI URL
# ---------------------------------------------------------------------------
$comfyUrl = if ($env:NU_COMFYUI_URL) { $env:NU_COMFYUI_URL } else { "http://127.0.0.1:8188" }
Set-UserEnv "NU_COMFYUI_URL" $comfyUrl

$comfyOk = $false
try {
    $resp = Invoke-WebRequest -Uri ($comfyUrl.TrimEnd("/") + "/system_stats") -UseBasicParsing -TimeoutSec 3
    if ($resp.StatusCode -eq 200) {
        Write-Host "[ok] ComfyUI API reachable at $comfyUrl" -ForegroundColor Green
        $comfyOk = $true
    }
} catch {
    Write-Host "[--] ComfyUI not reachable at $comfyUrl yet" -ForegroundColor Yellow
    Write-Host "     Comfy Desktop tip:" -ForegroundColor Yellow
    Write-Host "     You need a LOCAL install (not only Comfy Cloud)."
    Write-Host "     1) Open Comfy Desktop -> create/install Local ComfyUI"
    Write-Host "     2) Launch it and wait until the graph UI loads"
    Write-Host "     3) Re-check: Invoke-WebRequest http://127.0.0.1:8188/system_stats"
    Write-Host "     Shared models folder:"
    Write-Host "       $env:LOCALAPPDATA\Comfy-Desktop\ComfyUI-Shared\models"
}

# ---------------------------------------------------------------------------
# Python CLI (nu-content)
# ---------------------------------------------------------------------------
Write-Host ""
Write-Host "Installing nu-content CLI ..." -ForegroundColor Cyan
$py = Get-Command python -ErrorAction SilentlyContinue
if (-not $py) {
    Write-Host "[!!] Python not on PATH. Install Python 3.11+ and re-run." -ForegroundColor Red
} else {
    Write-Host "[ok] $($py.Source)"
    Push-Location (Join-Path $Root "tools\content")
    try {
        python -m pip install --upgrade pip -q
        python -m pip install -e . -q
        Write-Host "[ok] nu-content installed (pip install -e tools/content)" -ForegroundColor Green
    } finally {
        Pop-Location
    }

    Write-Host "     Checking nu-content --help ..."
    nu-content --help | Select-Object -First 5
}

# ---------------------------------------------------------------------------
# Blender smoke (scripts exist)
# ---------------------------------------------------------------------------
$blendScripts = @(
    "tools\content\blender\process.py",
    "tools\content\blender\export.py",
    "tools\content\blender\thumbnail.py"
)
foreach ($rel in $blendScripts) {
    $p = Join-Path $Root $rel
    if (Test-Path $p) {
        Write-Host "[ok] $rel"
    } else {
        Write-Host "[!!] Missing $rel" -ForegroundColor Red
    }
}

# ---------------------------------------------------------------------------
# Sample / practice prop
# ---------------------------------------------------------------------------
Write-Host ""
Write-Host "Ensuring practice prop exists ..." -ForegroundColor Cyan
$practiceToml = Join-Path $Root "assets\source\prop\prop_practice_cube\asset.toml"
if (-not (Test-Path $practiceToml)) {
    Write-Host "Creating practice asset prop_practice_cube ..."
    nu-content make-primitive prop_practice_cube --category prop --size 1.0
    Write-Host "[ok] Practice asset ready - next: nu-content process prop_practice_cube" -ForegroundColor Green
} else {
    Write-Host "[ok] Practice asset already exists (prop_practice_cube)"
}

# ---------------------------------------------------------------------------
# Summary
# ---------------------------------------------------------------------------
Write-Host ""
Write-Host "== Summary ==" -ForegroundColor Cyan
Write-Host "How-to guide:  docs\HOWTO_3D_PIPELINE.md"
Write-Host "Reference:     docs\ASSET_PIPELINE.md"
Write-Host ""
# Helpful for Windows consoles printing nu-content Unicode
Set-UserEnv "PYTHONUTF8" "1"

Write-Host "NEXT STEPS:" -ForegroundColor Yellow
Write-Host "  1. Close this terminal and open a NEW PowerShell (so env vars load)."
Write-Host "  2. Open docs\HOWTO_3D_PIPELINE.md and follow section 1.3 (Comfy)."
Write-Host "  3. In Comfy Desktop: create + launch a LOCAL ComfyUI (not only Cloud)."
if (-not $comfyOk) {
    Write-Host "  4. Verify API:  Invoke-WebRequest http://127.0.0.1:8188/system_stats"
}
Write-Host "  5. Follow docs/HOWTO_3D_PIPELINE.md free path (SD1.5 concepts + HF Space GLB)."
Write-Host "  6. Day-one dry run (no AI needed yet):"
Write-Host "       nu-content process prop_practice_cube"
Write-Host "       nu-content export prop_practice_cube"
Write-Host "       nu-content validate prop_practice_cube"
Write-Host ""
Write-Host "Done." -ForegroundColor Green

# Open the how-to in the default Markdown / editor if possible
$howto = Join-Path $Root "docs\HOWTO_3D_PIPELINE.md"
if (Test-Path $howto) {
    Write-Host "Opening HOWTO ..."
    Start-Process $howto
}
