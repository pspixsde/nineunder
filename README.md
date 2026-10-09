# Nine Under

A 3D action-roguelite from a third-person perspective, set in the Dread Pit.
Built on a custom C++20 / Vulkan 1.3 engine for maximum flexibility.

World and character notes live in [`LORE.md`](LORE.md).
Mechanical changes are tracked in [`CHANGELOG.md`](CHANGELOG.md).
Engine architecture is described in [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md).

## Prerequisites

| Tool | Notes |
|------|-------|
| **Visual Studio 2022** | Desktop development with C++ workload (MSVC + Windows SDK) |
| **CMake** ≥ 3.28 | With Ninja generator recommended |
| **Ninja** | Install via `winget install Ninja-build.Ninja` or VS installer |
| **Vulkan SDK** ≥ 1.3 | [LunarG](https://vulkan.lunarg.com/) — validation layers + tools |
| **vcpkg** | Clone once; set `VCPKG_ROOT` (see below) |
| **Git LFS** | `git lfs install` for binary assets |
| **Slang** (optional) | Shader compiler; also available via vcpkg `shader-slang` |

## Bootstrap

```powershell
# Helper (Git LFS + optional vcpkg clone)
.\scripts\bootstrap.ps1

# Or manually:
git lfs install
git clone https://github.com/microsoft/vcpkg.git C:\dev\vcpkg
C:\dev\vcpkg\bootstrap-vcpkg.bat
$env:VCPKG_ROOT = "C:\dev\vcpkg"
```

Install the [Vulkan SDK](https://vulkan.lunarg.com/) (or `winget install KhronosGroup.VulkanSDK`)
and ensure `VULKAN_SDK` is set. Add `VCPKG_ROOT` permanently via System Environment Variables.

Prebuilt SPIR-V for the mesh pass lives in `shaders/*.spv`. To rebuild:

```powershell
.\tools\compile_shaders.ps1
```

## Build

```powershell
# Configure (dev = optimized + asserts + Vulkan validation)
cmake --preset dev

# Build
cmake --build --preset dev

# Run
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
