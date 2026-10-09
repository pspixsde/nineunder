#pragma once

#include "nu/render/material.hpp"
#include "nu/render/mesh.hpp"

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace nu {

struct GltfMeshData {
    std::string name;
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    MaterialGpu material{};
};

struct GltfSceneData {
    std::vector<GltfMeshData> meshes;
};

[[nodiscard]] std::optional<GltfSceneData> loadGltf(const std::filesystem::path& path);

} // namespace nu
