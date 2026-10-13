#include "nu/physics/character_controller.hpp"

#include "nu/core/log.hpp"

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Character/CharacterVirtual.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/PhysicsSystem.h>

#include <algorithm>

namespace nu {

struct CharacterController::Impl {
    PhysicsWorld* world = nullptr;
    JPH::CharacterVirtual* character = nullptr;
    JPH::RefConst<JPH::Shape> standingShape;
    CharacterDesc desc{};
    Vec3 velocity{0.0f};
    float speedMul = 1.0f;
};

struct SharedPhysics {
    JPH::PhysicsSystem* system = nullptr;
    JPH::TempAllocator* allocator = nullptr;
};

static SharedPhysics g_sharedPhysics;

void registerPhysicsSystemForCharacter(JPH::PhysicsSystem* system, JPH::TempAllocator* allocator) {
    g_sharedPhysics.system = system;
    g_sharedPhysics.allocator = allocator;
}

CharacterController::CharacterController() : impl_(std::make_unique<Impl>()) {}

CharacterController::~CharacterController() {
    shutdown();
}

void CharacterController::init(PhysicsWorld& world, const CharacterDesc& desc) {
    impl_->world = &world;
    impl_->desc = desc;

    if (!g_sharedPhysics.system) {
        NU_LOG_WARN("CharacterController: physics system not registered yet; using kinematic fallback");
    }

    const float capsuleHalf = std::max(0.05f, desc.halfHeight);
    impl_->standingShape = new JPH::CapsuleShape(capsuleHalf, desc.radius);

    if (g_sharedPhysics.system) {
        JPH::CharacterVirtualSettings settings;
        settings.mShape = impl_->standingShape;
        settings.mSupportingVolume = JPH::Plane(JPH::Vec3::sAxisY(), -desc.radius);
        settings.mMaxSlopeAngle = JPH::DegreesToRadians(50.0f);
        settings.mMass = 80.0f;

        impl_->character = new JPH::CharacterVirtual(
            &settings, JPH::RVec3(desc.initialPosition.x, desc.initialPosition.y, desc.initialPosition.z),
            JPH::Quat::sIdentity(), 0, g_sharedPhysics.system);
    }

    NU_LOG_INFO("CharacterController ready at ({}, {}, {})", desc.initialPosition.x, desc.initialPosition.y,
                desc.initialPosition.z);
}

void CharacterController::shutdown() {
    if (impl_->character) {
        delete impl_->character;
        impl_->character = nullptr;
    }
    impl_->standingShape = nullptr;
    impl_->world = nullptr;
}

void CharacterController::update(float dt, const Vec3& wishDir, bool jump) {
    Vec3 dir = wishDir;
    const float len = glm::length(Vec2{dir.x, dir.z});
    if (len > 1.0f) {
        dir.x /= len;
        dir.z /= len;
    }

    const float speed = impl_->desc.moveSpeed * std::clamp(impl_->speedMul, 0.05f, 3.0f);

    if (impl_->character && g_sharedPhysics.system && g_sharedPhysics.allocator) {
        const JPH::Vec3 joltWish(dir.x, 0.0f, dir.z);
        JPH::CharacterVirtual::ExtendedUpdateSettings updateSettings;

        JPH::Vec3 velocity = impl_->character->GetLinearVelocity();
        if (impl_->character->GetGroundState() == JPH::CharacterVirtual::EGroundState::OnGround) {
            velocity = JPH::Vec3(joltWish.GetX() * speed, 0.0f, joltWish.GetZ() * speed);
            if (jump) {
                velocity.SetY(impl_->desc.jumpSpeed);
            }
        } else {
            velocity.SetX(joltWish.GetX() * speed * 0.85f);
            velocity.SetZ(joltWish.GetZ() * speed * 0.85f);
            velocity += g_sharedPhysics.system->GetGravity() * dt;
        }
        impl_->character->SetLinearVelocity(velocity);

        impl_->character->ExtendedUpdate(dt, g_sharedPhysics.system->GetGravity(), updateSettings,
                                         g_sharedPhysics.system->GetDefaultBroadPhaseLayerFilter(0),
                                         g_sharedPhysics.system->GetDefaultLayerFilter(0), {}, {},
                                         *g_sharedPhysics.allocator);

        const JPH::Vec3 v = impl_->character->GetLinearVelocity();
        impl_->velocity = {v.GetX(), v.GetY(), v.GetZ()};
        return;
    }

    // Kinematic fallback.
    impl_->velocity.x = dir.x * speed;
    impl_->velocity.z = dir.z * speed;
    impl_->velocity.y -= 18.0f * dt;
    if (jump && onGround()) {
        impl_->velocity.y = impl_->desc.jumpSpeed;
    }
    impl_->desc.initialPosition += impl_->velocity * dt;
    if (impl_->desc.initialPosition.y < 1.0f) {
        impl_->desc.initialPosition.y = 1.0f;
        impl_->velocity.y = 0.0f;
    }
}

void CharacterController::setPosition(const Vec3& position) {
    impl_->velocity = {};
    impl_->desc.initialPosition = position;
    if (impl_->character) {
        impl_->character->SetPosition(JPH::RVec3(position.x, position.y, position.z));
        impl_->character->SetLinearVelocity(JPH::Vec3::sZero());
    }
}

void CharacterController::setMoveSpeedMultiplier(float multiplier) {
    impl_->speedMul = multiplier;
}

Vec3 CharacterController::position() const {
    if (impl_->character) {
        const JPH::RVec3 p = impl_->character->GetPosition();
        return {static_cast<float>(p.GetX()), static_cast<float>(p.GetY()), static_cast<float>(p.GetZ())};
    }
    return impl_->desc.initialPosition;
}

Vec3 CharacterController::velocity() const {
    return impl_->velocity;
}

bool CharacterController::onGround() const {
    if (impl_->character) {
        return impl_->character->GetGroundState() == JPH::CharacterVirtual::EGroundState::OnGround;
    }
    return impl_->desc.initialPosition.y <= 1.01f;
}

} // namespace nu
