#pragma once

#include "nu/math/math.hpp"

#include <string>

namespace nu {

struct TransformComponent {
    Vec3 position{0.0f};
    Quat rotation{1.0f, 0.0f, 0.0f, 0.0f};
    Vec3 scale{1.0f};

    [[nodiscard]] Mat4 matrix() const {
        Mat4 t = glm::translate(Mat4{1.0f}, position);
        Mat4 r = glm::mat4_cast(rotation);
        Mat4 s = glm::scale(Mat4{1.0f}, scale);
        return t * r * s;
    }
};

struct NameComponent {
    std::string name;
};

} // namespace nu
