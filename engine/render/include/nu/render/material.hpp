#pragma once

#include "nu/core/types.hpp"
#include "nu/math/math.hpp"

namespace nu {

struct MaterialGpu {
    Vec4 baseColor{0.7f, 0.7f, 0.72f, 1.0f};
    float metallic = 0.0f;
    float roughness = 0.85f;
    float pad0 = 0.0f;
    float pad1 = 0.0f;
    Vec4 emissive{0.0f, 0.0f, 0.0f, 0.0f};
};

struct FrameConstants {
    Mat4 viewProj;
    Mat4 view;
    Vec4 cameraPos;      // xyz
    Vec4 lightDir;       // xyz normalized, w unused
    Vec4 lightColor;     // rgb intensity
    Vec4 fogColor;       // rgb + density in w
    Vec4 ambientColor;   // rgb
};

} // namespace nu
