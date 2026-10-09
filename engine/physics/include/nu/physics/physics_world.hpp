#pragma once

#include "nu/math/math.hpp"

#include <cstdint>
#include <memory>
#include <vector>

namespace nu {

struct PhysicsBodyId {
    uint32_t index = 0;
    bool valid = false;
};

class PhysicsWorld {
public:
    PhysicsWorld();
    ~PhysicsWorld();

    PhysicsWorld(const PhysicsWorld&) = delete;
    PhysicsWorld& operator=(const PhysicsWorld&) = delete;

    void init();
    void shutdown();
    void update(float dt);

    PhysicsBodyId addBox(const Vec3& halfExtents, const Vec3& position, bool staticBody = true);
    PhysicsBodyId addPlane(const Vec3& position, float size = 100.0f);

    [[nodiscard]] Vec3 getBodyPosition(PhysicsBodyId id) const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace nu
