#include "nu/render/material.hpp"

// MaterialGpu / FrameConstants are POD shared with shaders; no runtime logic yet.
namespace nu {
namespace {
[[maybe_unused]] constexpr int kMaterialModule = 1;
}
} // namespace nu
