# Verify / ease Comfy Desktop local setup for Nine Under (Windows).
# Run while Local ComfyUI is launched.

$ErrorActionPreference = "Continue"
$Root = Split-Path -Parent $PSScriptRoot
$ComfyUrl = if ($env:NU_COMFYUI_URL) { $env:NU_COMFYUI_URL.TrimEnd("/") } else { "http://127.0.0.1:8188" }
$Shared = Join-Path $env:LOCALAPPDATA "Comfy-Desktop\ComfyUI-Shared"
$CkptDir = Join-Path $Shared "models\checkpoints"
$OutDir = Join-Path $Shared "output"
$CustomNodes = Join-Path $env:LOCALAPPDATA "Comfy-Desktop\ComfyUI-Installs\ComfyUI\ComfyUI\custom_nodes"

Write-Host "== Comfy Desktop helper ==" -ForegroundColor Cyan
Write-Host "API: $ComfyUrl"

# ---------------------------------------------------------------------------
# API ping
# ---------------------------------------------------------------------------
$apiOk = $false
try {
    $stats = Invoke-RestMethod -Uri "$ComfyUrl/system_stats" -TimeoutSec 8
    $apiOk = $true
    Write-Host "[ok] ComfyUI API up" -ForegroundColor Green
    Write-Host ("     version={0} pytorch={1}" -f $stats.system.comfyui_version, $stats.system.pytorch_version)
    if ($stats.devices) {
        foreach ($d in $stats.devices) {
            Write-Host ("     device={0} vram_total_mb={1:N0}" -f $d.name, ($d.vram_total / 1MB))
        }
    }
} catch {
    Write-Host "[!!] API not reachable. Launch Local ComfyUI in Desktop first." -ForegroundColor Red
    Write-Host "     $($_.Exception.Message)"
}

# ---------------------------------------------------------------------------
# Checkpoint (SD1.5 for 8 GB concepts)
# ---------------------------------------------------------------------------
New-Item -ItemType Directory -Force -Path $CkptDir | Out-Null
$ckpt = Join-Path $CkptDir "v1-5-pruned-emaonly.safetensors"
if (Test-Path $ckpt) {
    $mb = [math]::Round((Get-Item $ckpt).Length / 1MB)
    if ($mb -lt 3000) {
        Write-Host "[--] Checkpoint present but small ($mb MB) - download may still be running" -ForegroundColor Yellow
    } else {
        Write-Host "[ok] Checkpoint ready: v1-5-pruned-emaonly.safetensors ($mb MB)" -ForegroundColor Green
    }
} else {
    Write-Host "[--] Missing SD1.5 checkpoint. Starting download (~4 GB)..." -ForegroundColor Yellow
    $url = "https://huggingface.co/stable-diffusion-v1-5/stable-diffusion-v1-5/resolve/main/v1-5-pruned-emaonly.safetensors"
    Start-Process -FilePath "curl.exe" -ArgumentList @("-L","--retry","3","-C","-","-o",$ckpt,$url) -WindowStyle Normal
    Write-Host "     Download started in a new window. Re-run this script when finished."
}

# ---------------------------------------------------------------------------
# Useful paths
# ---------------------------------------------------------------------------
Write-Host ""
Write-Host "Folders:" -ForegroundColor Cyan
Write-Host "  checkpoints: $CkptDir"
Write-Host "  output:      $OutDir"
Write-Host "  custom_nodes:$CustomNodes"
Write-Host "  workflows:   $Root\tools\content\comfyui\workflows"

# ---------------------------------------------------------------------------
# What Desktop already provides
# ---------------------------------------------------------------------------
Write-Host ""
Write-Host "Built into your Comfy Desktop (no Manager install needed for these):" -ForegroundColor Cyan
Write-Host "  - SaveGLB / MeshToFile3D / Preview3D"
Write-Host "  - Mesh cleanup: DecimateMesh, RemeshMesh, UnwrapMesh"
Write-Host "  - Template: Workflows -> Pixal3D & TRELLIS.2 (local free image->3D; slow on 8 GB)"
Write-Host "  - Tripo* / Meshy* nodes = cloud API (credits) — skip for free pipeline"
Write-Host "  - RemoveBackground / LoadBackgroundRemovalModel available for cleanup"

Write-Host ""
Write-Host "Recommended FREE canvas path (see docs/HOWTO_3D_PIPELINE.md):" -ForegroundColor Yellow
Write-Host "  1. Text->image: your 'Nine Under v1' graph (SD1.5 or SDXL)"
Write-Host "  2. Image->3D: Workflow template 'Pixal3D & TRELLIS.2' (local, free)"
Write-Host "     Skip Comfy 'Tripo: Image to Model' - that is paid cloud credits"
Write-Host "  3. Copy GLB from Shared\output into assets\source\...\raw\selected.glb"
Write-Host "     then polish/export in Blender UI"

# ---------------------------------------------------------------------------
# nu-content ping
# ---------------------------------------------------------------------------
Write-Host ""
if (Get-Command nu-content -ErrorAction SilentlyContinue) {
    Write-Host "[ok] nu-content on PATH"
    nu-content backends 2>$null | Select-Object -First 12
} else {
    Write-Host "[--] nu-content not on PATH in this shell. Run setup_content_pipeline.ps1 or open a new terminal."
}

if ($apiOk) {
    Write-Host ""
    Write-Host "Optional: queue Desktop concept workflow after checkpoint finishes:" -ForegroundColor Cyan
    Write-Host "  nu-content new prop_desk_test --category prop --prompt `"cracked ruin stone cube`""
    Write-Host "  (then set concept.workflow = concept_prop_desktop_v1 in asset.toml)"
    Write-Host "  nu-content concept prop_desk_test"
}

Write-Host ""
Write-Host "Done." -ForegroundColor Green
