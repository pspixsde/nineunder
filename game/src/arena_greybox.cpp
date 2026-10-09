#include "arena_greybox.hpp"

#include "nu/core/log.hpp"

namespace nine {

ArenaGreybox buildArenaOneGreybox(nu::MeshFactory& meshes, nu::PhysicsWorld& physics, const nu::GpuMesh& floorMesh,
                                  const nu::GpuMesh& cubeMesh, const nu::GpuMesh& wallMesh) {
    (void)meshes;
    ArenaGreybox arena;

    // Floating island platform floor
    physics.addPlane({0.0f, 0.0f, 0.0f}, 80.0f);

    nu::DrawItem floor;
    floor.mesh = &floorMesh;
    floor.transform = nu::Mat4{1.0f};
    floor.material.baseColor = {0.42f, 0.44f, 0.48f, 1.0f};
    floor.material.roughness = 0.95f;
    arena.draws.push_back(floor);

    // Cobble/bone wall toward Sun-Glint Vale (+Z)
    for (int i = -12; i <= 12; ++i) {
        nu::DrawItem wall;
        wall.mesh = &wallMesh;
        wall.transform = glm::translate(nu::Mat4{1.0f}, nu::Vec3{static_cast<float>(i) * 1.5f, 2.0f, -28.0f}) *
                         glm::scale(nu::Mat4{1.0f}, nu::Vec3{1.5f, 4.0f, 0.6f});
        wall.material.baseColor = {0.55f, 0.5f, 0.45f, 1.0f};
        wall.material.roughness = 0.9f;
        arena.draws.push_back(wall);
        physics.addBox({0.75f, 2.0f, 0.3f}, {static_cast<float>(i) * 1.5f, 2.0f, -28.0f}, true);
    }

    // Gate (closed) in the middle of the wall
    {
        nu::DrawItem gate;
        gate.mesh = &cubeMesh;
        gate.transform = glm::translate(nu::Mat4{1.0f}, nu::Vec3{0.0f, 2.5f, -27.2f}) *
                         glm::scale(nu::Mat4{1.0f}, nu::Vec3{3.0f, 5.0f, 0.4f});
        gate.material.baseColor = {0.25f, 0.22f, 0.2f, 1.0f};
        gate.material.metallic = 0.4f;
        gate.material.roughness = 0.55f;
        arena.draws.push_back(gate);
    }

    // Ink river hint along -Z shore (dark slabs)
    for (int i = -10; i <= 10; ++i) {
        nu::DrawItem ink;
        ink.mesh = &cubeMesh;
        ink.transform = glm::translate(nu::Mat4{1.0f}, nu::Vec3{static_cast<float>(i) * 2.0f, -0.4f, 30.0f}) *
                        glm::scale(nu::Mat4{1.0f}, nu::Vec3{2.0f, 0.2f, 8.0f});
        ink.material.baseColor = {0.05f, 0.05f, 0.08f, 1.0f};
        ink.material.roughness = 0.35f;
        ink.material.metallic = 0.1f;
        arena.draws.push_back(ink);
    }

    // Scattered ruins / cracked structures
    const nu::Vec3 ruins[] = {{-8.0f, 0.5f, 6.0f}, {10.0f, 0.5f, -4.0f}, {-14.0f, 0.5f, -10.0f},
                              {6.0f, 0.5f, 12.0f}, {-3.0f, 0.5f, -16.0f}, {16.0f, 0.5f, 8.0f}};
    for (const nu::Vec3& pos : ruins) {
        nu::DrawItem block;
        block.mesh = &cubeMesh;
        block.transform = glm::translate(nu::Mat4{1.0f}, pos) * glm::scale(nu::Mat4{1.0f}, nu::Vec3{2.0f, 1.0f, 2.0f});
        block.material.baseColor = {0.5f, 0.48f, 0.46f, 1.0f};
        arena.draws.push_back(block);
        physics.addBox({1.0f, 0.5f, 1.0f}, pos, true);
    }

    // Diluted ink pools (hazard placeholders — dark discs as flat cubes)
    const nu::Vec3 pools[] = {{4.0f, 0.05f, 4.0f}, {-10.0f, 0.05f, 2.0f}, {8.0f, 0.05f, -8.0f}};
    for (const nu::Vec3& pos : pools) {
        nu::DrawItem pool;
        pool.mesh = &cubeMesh;
        pool.transform = glm::translate(nu::Mat4{1.0f}, pos) * glm::scale(nu::Mat4{1.0f}, nu::Vec3{3.0f, 0.05f, 3.0f});
        pool.material.baseColor = {0.08f, 0.06f, 0.12f, 1.0f};
        pool.material.roughness = 0.25f;
        arena.draws.push_back(pool);
    }

    NU_LOG_INFO("Arena I greybox: platform, wall/gate, ink shore, ruins, pools");
    return arena;
}

} // namespace nine
