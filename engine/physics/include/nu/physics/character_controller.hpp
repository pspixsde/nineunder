#pragma once

#include "nu/math/math.hpp"
#include "nu/physics/physics_world.hpp"

#include <memory>

namespace nu {

struct CharacterDesc {
    float radius = 0.4f;
    float halfHeight = 0.5f;
    float moveSpeed = 7.0f;
    float jumpSpeed = 8.0f;
    Vec3 initialPosition{0.0f, 2.0f, 0.0f};
};

class CharacterController {
public:
    CharacterController();
    ~CharacterController();

    void init(PhysicsWorld& world, const CharacterDesc& desc = {});
    void shutdown();

    /// wishDir is XZ movement in world space (already camera-relative), jump edge-triggered.
    void update(float dt, const Vec3& wishDir, bool jump);

    [[nodiscard]] Vec3 position() const;
    [[nodiscard]] Vec3 velocity() const;
    [[nodiscard]] bool onGround() const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace nu
