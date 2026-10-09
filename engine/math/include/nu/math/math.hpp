#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace nu {

using Vec2 = glm::vec2;
using Vec3 = glm::vec3;
using Vec4 = glm::vec4;
using Mat3 = glm::mat3;
using Mat4 = glm::mat4;
using Quat = glm::quat;
using IVec2 = glm::ivec2;
using UVec2 = glm::uvec2;

[[nodiscard]] inline Mat4 perspectiveVk(float fovyRadians, float aspect, float zNear, float zFar) {
    // GLM_FORCE_DEPTH_ZERO_TO_ONE already set; Vulkan clip space Y flip applied by caller if needed.
    return glm::perspective(fovyRadians, aspect, zNear, zFar);
}

[[nodiscard]] inline Mat4 lookAt(const Vec3& eye, const Vec3& center, const Vec3& up) {
    return glm::lookAt(eye, center, up);
}

[[nodiscard]] inline float lerp(float a, float b, float t) {
    return a + (b - a) * t;
}

[[nodiscard]] inline Vec3 lerp(const Vec3& a, const Vec3& b, float t) {
    return a + (b - a) * t;
}

} // namespace nu
