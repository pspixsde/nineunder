#include "arena_greybox.hpp"

#include "nu/core/log.hpp"

namespace nine {

ArenaGreybox buildArenaOneGreybox(nu::MeshFactory& meshes, nu::PhysicsWorld& physics, const nu::GpuMesh& floorMesh,
                                  const nu::GpuMesh& cubeMesh, const nu::GpuMesh& wallMesh) {
    (void)meshes;
    ArenaGreybox arena;

    // Larger floating island for testing / editor placement.
    physics.addPlane({0.0f, 0.0f, 0.0f}, 140.0f);

    nu::DrawItem floor;
    floor.mesh = &floorMesh;
    floor.transform = nu::Mat4{1.0f};
    floor.material.baseColor = {0.42f, 0.44f, 0.48f, 1.0f};
    floor.material.roughness = 0.95f;
    arena.draws.push_back(floor);

    // Cobble/bone wall toward Sun-Glint Vale (pushed further back with the larger floor)
    for (int i = -18; i <= 18; ++i) {
        nu::DrawItem wall;
        wall.mesh = &wallMesh;
        wall.transform = glm::translate(nu::Mat4{1.0f}, nu::Vec3{static_cast<float>(i) * 1.5f, 2.0f, -40.0f}) *
                         glm::scale(nu::Mat4{1.0f}, nu::Vec3{1.5f, 4.0f, 0.6f});
        wall.material.baseColor = {0.55f, 0.5f, 0.45f, 1.0f};
        wall.material.roughness = 0.9f;
        arena.draws.push_back(wall);
        physics.addBox({0.75f, 2.0f, 0.3f}, {static_cast<float>(i) * 1.5f, 2.0f, -40.0f}, true);
    }

    // Gate (closed) in the middle of the wall
    {
        nu::DrawItem gate;
        gate.mesh = &cubeMesh;
        gate.transform = glm::translate(nu::Mat4{1.0f}, nu::Vec3{0.0f, 2.5f, -39.2f}) *
                         glm::scale(nu::Mat4{1.0f}, nu::Vec3{3.0f, 5.0f, 0.4f});
        gate.material.baseColor = {0.25f, 0.22f, 0.2f, 1.0f};
        gate.material.metallic = 0.4f;
        gate.material.roughness = 0.55f;
        arena.draws.push_back(gate);
    }

    // Ink river hint along far shore
    for (int i = -16; i <= 16; ++i) {
        nu::DrawItem ink;
        ink.mesh = &cubeMesh;
        ink.transform = glm::translate(nu::Mat4{1.0f}, nu::Vec3{static_cast<float>(i) * 2.0f, -0.4f, 42.0f}) *
                        glm::scale(nu::Mat4{1.0f}, nu::Vec3{2.0f, 0.2f, 10.0f});
        ink.material.baseColor = {0.05f, 0.05f, 0.08f, 1.0f};
        ink.material.roughness = 0.35f;
        ink.material.metallic = 0.1f;
        arena.draws.push_back(ink);
    }

    // Ruins / ink pools come from ArenaMap (play + map editor), not the static greybox.
    NU_LOG_INFO("Arena I greybox: expanded platform, wall/gate, ink shore");
    return arena;
}

} // namespace nine
