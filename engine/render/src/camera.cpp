#include "nu/render/camera.hpp"

#include <algorithm>
#include <cmath>

namespace nu {

void Camera::setPerspective(float fovyDegrees, float aspect, float zNear, float zFar) {
    fovy_ = fovyDegrees;
    aspect_ = aspect;
    zNear_ = zNear;
    zFar_ = zFar;
    projection_ = perspectiveVk(glm::radians(fovy_), aspect_, zNear_, zFar_);
    projection_[1][1] *= -1.0f; // Vulkan Y flip
}

void Camera::setAspect(float aspect) {
    aspect_ = aspect;
    projection_ = perspectiveVk(glm::radians(fovy_), aspect_, zNear_, zFar_);
    projection_[1][1] *= -1.0f;
}

void Camera::addYawPitch(float yawDeg, float pitchDeg) {
    yaw += yawDeg;
    pitch = std::clamp(pitch + pitchDeg, -89.0f, 89.0f);
}

void Camera::moveLocal(const Vec3& localDelta) {
    position_ += right() * localDelta.x;
    position_ += Vec3{0.0f, 1.0f, 0.0f} * localDelta.y;
    position_ += forward() * localDelta.z;
}

void Camera::addOrbit(float yawDeg, float pitchDeg) {
    yaw += yawDeg;
    pitch = std::clamp(pitch + pitchDeg, -89.0f, 89.0f);
}

void Camera::addDistance(float delta) {
    distance_ = std::clamp(distance_ + delta, 1.5f, 40.0f);
}

Vec3 Camera::forward() const {
    const float yawR = glm::radians(yaw);
    const float pitchR = glm::radians(pitch);
    return glm::normalize(Vec3{std::cos(yawR) * std::cos(pitchR), std::sin(pitchR), std::sin(yawR) * std::cos(pitchR)});
}

Vec3 Camera::right() const {
    return glm::normalize(glm::cross(forward(), Vec3{0.0f, 1.0f, 0.0f}));
}

void Camera::update() {
    if (mode_ == CameraMode::ThirdPerson) {
        const Vec3 offset = -forward() * distance_;
        position_ = target_ + offset + Vec3{0.0f, 1.2f, 0.0f};
        view_ = lookAt(position_, target_ + Vec3{0.0f, 1.0f, 0.0f}, Vec3{0.0f, 1.0f, 0.0f});
    } else {
        view_ = lookAt(position_, position_ + forward(), Vec3{0.0f, 1.0f, 0.0f});
    }
}

} // namespace nu
