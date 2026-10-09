# Nine Under

A 3D action-roguelite from a third-person perspective, set in the Dread Pit.
Built on a custom C++20 / Vulkan 1.3 engine for maximum flexibility.

World and character notes live in [`LORE.md`](LORE.md).
Mechanical changes are tracked in [`CHANGELOG.md`](CHANGELOG.md).
Engine architecture is described in [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md).

## Prerequisites

| Tool | Notes |
|------|-------|
| **Visual Studio 2022** (or Build Tools) | **MSVC + Windows SDK** — required. LLVM/MinGW alone is not enough for the `x64-windows` vcpkg triplet |
| **CMake** ≥ 3.28 | |
| **Ninja** | `winget install Ninja-build.Ninja` |
| **Vulkan SDK** ≥ 1.3 | `winget install KhronosGroup.VulkanSDK` |
| **vcpkg** | Set `VCPKG_ROOT` (bootstrap clones to `C:\dev\vcpkg`) |
| **Git LFS** | `git lfs install` |

## Bootstrap

```powershell
.\scripts\bootstrap.ps1
```

That script clones vcpkg if needed and checks for MSVC / Ninja / Vulkan.

If MSVC is missing:

```powershell
winget install Microsoft.VisualStudio.2022.BuildTools --override "--wait --passive --add Microsoft.VisualStudio.Workload.VCTools --includeRecommended"
```

Then open **x64 Native Tools Command Prompt for VS 2022** (or VS Developer PowerShell) so `cl.exe` is on `PATH`.

```powershell
# Permanent env (System Properties → Environment Variables), or per session:
$env:VCPKG_ROOT = "C:\dev\vcpkg"
```

Prebuilt SPIR-V lives in `shaders/*.spv`. Rebuild with `.\tools\compile_shaders.ps1`.

## Build

Run from a **VS x64 developer shell**:

```powershell
$env:VCPKG_ROOT = "C:\dev\vcpkg"   # if not set permanently
cmake --preset dev
cmake --build --preset dev
.\build\dev\game\NineUnder.exe
```

Presets: `debug` (unoptimized + ASan), `dev` (day-to-day), `release` (shipping).

## Tests

```powershell
cmake --build --preset dev --target nu_tests
ctest --preset dev
```

## Layout

```
engine/     # static lib nu_engine — game-agnostic
game/       # NineUnder.exe — gameplay
tools/      # asset cooker, shader helpers
shaders/    # Slang sources → SPIR-V
assets/     # source art (Git LFS)
tests/      # doctest unit tests
docs/       # architecture and conventions
```

`engine/` must never include headers from `game/`.

## License

All rights reserved for now.
