#include "nu/physics/physics_world.hpp"

#include "nu/core/log.hpp"

#include <Jolt/Jolt.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/RegisterTypes.h>

#include <algorithm>
#include <cstdarg>
#include <cstdio>
#include <thread>

namespace nu {

// Defined in character_controller.cpp
void registerPhysicsSystemForCharacter(JPH::PhysicsSystem* system, JPH::TempAllocator* allocator);

namespace {

static void joltTrace(const char* fmt, ...) {
    char buffer[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);
    NU_LOG_TRACE("[Jolt] {}", buffer);
}

#ifdef JPH_ENABLE_ASSERTS
static bool joltAssert(const char* expr, const char* msg, const char* file, uint32_t line) {
    NU_LOG_ERROR("Jolt assert: {} {} ({}:{})", expr, msg ? msg : "", file, line);
    return true;
}
#endif

} // namespace

struct PhysicsWorld::Impl {
    JPH::TempAllocatorImpl* tempAllocator = nullptr;
    JPH::JobSystemThreadPool* jobSystem = nullptr;
    JPH::PhysicsSystem physicsSystem;
    JPH::BodyInterface* bodyInterface = nullptr;
    bool registered = false;

    struct BodyRecord {
        JPH::BodyID id;
        bool alive = false;
    };
    std::vector<BodyRecord> bodies;
};

PhysicsWorld::PhysicsWorld() : impl_(std::make_unique<Impl>()) {}

PhysicsWorld::~PhysicsWorld() {
    shutdown();
}

void PhysicsWorld::init() {
    if (!impl_->registered) {
        JPH::RegisterDefaultAllocator();
        JPH::Factory::sInstance = new JPH::Factory();
        JPH::RegisterTypes();
        JPH::Trace = joltTrace;
#ifdef JPH_ENABLE_ASSERTS
        JPH::AssertFailed = joltAssert;
#endif
        impl_->registered = true;
    }

    impl_->tempAllocator = new JPH::TempAllocatorImpl(32 * 1024 * 1024);
    const unsigned threads = std::max(1u, std::thread::hardware_concurrency() - 1);
    impl_->jobSystem = new JPH::JobSystemThreadPool(JPH::cMaxPhysicsJobs, JPH::cMaxPhysicsBarriers, threads);

    constexpr JPH::uint cMaxBodies = 4096;
    constexpr JPH::uint cNumBodyMutexes = 0;
    constexpr JPH::uint cMaxBodyPairs = 65536;
    constexpr JPH::uint cMaxContactConstraints = 10240;

    // Broadphase / object layers — simple single-layer setup for graybox.
    class BPLayerInterface final : public JPH::BroadPhaseLayerInterface {
    public:
        JPH::uint GetNumBroadPhaseLayers() const override { return 1; }
        JPH::BroadPhaseLayer GetBroadPhaseLayer(JPH::ObjectLayer) const override {
            return JPH::BroadPhaseLayer(0);
        }
#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
        const char* GetBroadPhaseLayerName(JPH::BroadPhaseLayer) const override { return "Default"; }
#endif
    };

    class ObjectVsBP final : public JPH::ObjectVsBroadPhaseLayerFilter {
    public:
        bool ShouldCollide(JPH::ObjectLayer, JPH::BroadPhaseLayer) const override { return true; }
    };

    class ObjectLayerPair final : public JPH::ObjectLayerPairFilter {
    public:
        bool ShouldCollide(JPH::ObjectLayer, JPH::ObjectLayer) const override { return true; }
    };

    static BPLayerInterface bpLayers;
    static ObjectVsBP objVsBp;
    static ObjectLayerPair objPair;

    impl_->physicsSystem.Init(cMaxBodies, cNumBodyMutexes, cMaxBodyPairs, cMaxContactConstraints, bpLayers, objVsBp,
                              objPair);
    impl_->physicsSystem.SetGravity(JPH::Vec3(0, -18.0f, 0));
    impl_->bodyInterface = &impl_->physicsSystem.GetBodyInterface();
    registerPhysicsSystemForCharacter(&impl_->physicsSystem, impl_->tempAllocator);

    NU_LOG_INFO("Jolt physics initialized ({} job threads)", threads);
}

void PhysicsWorld::shutdown() {
    if (!impl_) {
        return;
    }
    registerPhysicsSystemForCharacter(nullptr, nullptr);
    if (impl_->bodyInterface) {
        for (auto& b : impl_->bodies) {
            if (b.alive) {
                impl_->bodyInterface->RemoveBody(b.id);
                impl_->bodyInterface->DestroyBody(b.id);
                b.alive = false;
            }
        }
        impl_->bodies.clear();
    }
    delete impl_->jobSystem;
    impl_->jobSystem = nullptr;
    delete impl_->tempAllocator;
    impl_->tempAllocator = nullptr;
    impl_->bodyInterface = nullptr;
}

void PhysicsWorld::update(float dt) {
    if (!impl_->tempAllocator) {
        return;
    }
    impl_->physicsSystem.Update(dt, 1, impl_->tempAllocator, impl_->jobSystem);
}

PhysicsBodyId PhysicsWorld::addBox(const Vec3& halfExtents, const Vec3& position, bool staticBody) {
    JPH::BoxShapeSettings shapeSettings(JPH::Vec3(halfExtents.x, halfExtents.y, halfExtents.z));
    shapeSettings.SetEmbedded();
    JPH::ShapeSettings::ShapeResult shapeResult = shapeSettings.Create();
    JPH::ShapeRefC shape = shapeResult.Get();

    JPH::BodyCreationSettings settings(
        shape, JPH::RVec3(position.x, position.y, position.z), JPH::Quat::sIdentity(),
        staticBody ? JPH::EMotionType::Static : JPH::EMotionType::Dynamic, 0);
    JPH::Body* body = impl_->bodyInterface->CreateBody(settings);
    impl_->bodyInterface->AddBody(body->GetID(), JPH::EActivation::DontActivate);

    PhysicsBodyId id{};
    id.index = static_cast<uint32_t>(impl_->bodies.size());
    id.valid = true;
    impl_->bodies.push_back({body->GetID(), true});
    return id;
}

PhysicsBodyId PhysicsWorld::addPlane(const Vec3& position, float size) {
    return addBox(Vec3{size * 0.5f, 0.25f, size * 0.5f}, position - Vec3{0.0f, 0.25f, 0.0f}, true);
}

Vec3 PhysicsWorld::getBodyPosition(PhysicsBodyId id) const {
    if (!id.valid || id.index >= impl_->bodies.size() || !impl_->bodies[id.index].alive) {
        return {};
    }
    const JPH::RVec3 p = impl_->bodyInterface->GetCenterOfMassPosition(impl_->bodies[id.index].id);
    return {static_cast<float>(p.GetX()), static_cast<float>(p.GetY()), static_cast<float>(p.GetZ())};
}

} // namespace nu
