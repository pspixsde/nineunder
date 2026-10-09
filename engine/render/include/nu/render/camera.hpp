#pragma once

#include "nu/math/math.hpp"

namespace nu {

enum class CameraMode {
    Free,
    ThirdPerson
};

class Camera {
public:
    void setPerspective(float fovyDegrees, float aspect, float zNear, float zFar);
    void setAspect(float aspect);

    void setMode(CameraMode mode) { mode_ = mode; }
    [[nodiscard]] CameraMode mode() const { return mode_; }

    /// Free-fly: position + yaw/pitch (degrees).
    void setPosition(const Vec3& pos) { position_ = pos; }
    [[nodiscard]] Vec3 position() const { return position_; }

    void addYawPitch(float yawDeg, float pitchDeg);
    void moveLocal(const Vec3& localDelta);

    /// Third-person: orbit around a target.
    void setTarget(const Vec3& target) { target_ = target; }
    void setDistance(float distance) { distance_ = std::max(1.0f, distance); }
    void addOrbit(float yawDeg, float pitchDeg);
    void addDistance(float delta);

    void update();

    [[nodiscard]] const Mat4& view() const { return view_; }
    [[nodiscard]] const Mat4& projection() const { return projection_; }
    [[nodiscard]] Mat4 viewProjection() const { return projection_ * view_; }
    [[nodiscard]] Vec3 forward() const;
    [[nodiscard]] Vec3 right() const;

    float yaw = -90.0f;
    float pitch = -20.0f;

private:
    CameraMode mode_ = CameraMode::ThirdPerson;
    Vec3 position_{0.0f, 2.0f, 6.0f};
    Vec3 target_{0.0f, 1.0f, 0.0f};
    float distance_ = 6.0f;
    float fovy_ = 60.0f;
    float aspect_ = 16.0f / 9.0f;
    float zNear_ = 0.1f;
    float zFar_ = 500.0f;
    Mat4 view_{1.0f};
    Mat4 projection_{1.0f};
};

} // namespace nu
