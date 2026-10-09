#pragma once

#include "nu/physics/physics_world.hpp"
#include "nu/render/renderer.hpp"

#include <vector>

namespace nine {

/// Procedural graybox for Arena I — The Grey Threshold.
struct ArenaGreybox {
    std::vector<nu::DrawItem> draws;
};

[[nodiscard]] ArenaGreybox buildArenaOneGreybox(nu::MeshFactory& meshes, nu::PhysicsWorld& physics,
                                                const nu::GpuMesh& floorMesh, const nu::GpuMesh& cubeMesh,
                                                const nu::GpuMesh& wallMesh);

} // namespace nine
