#pragma once

#include "nu/math/math.hpp"
#include "nu/render/material.hpp"
#include "nu/render/mesh.hpp"

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace nu {

struct GltfTextureData {
    std::string name;
    int width = 0;
    int height = 0;
    int channels = 4;
    std::vector<uint8_t> pixels; // RGBA8
    bool srgb = true;
};

struct GltfMaterialData {
    std::string name;
    MaterialGpu gpu{};
    int baseColorTexture = -1;
    int normalTexture = -1;
    int ormTexture = -1; // metallicRoughness (glTF) — treated as ORM-ish
    int emissiveTexture = -1;
};

struct GltfMeshData {
    std::string name;
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    int materialIndex = -1;
    int lod = 0; // parsed from _LOD# suffix
};

enum class GltfCollisionType { None, Box, Capsule, Mesh };

struct GltfCollisionNode {
    std::string name;
    GltfCollisionType type = GltfCollisionType::None;
    Vec3 translation{0.0f};
    Vec3 halfExtents{0.5f};
    float radius = 0.4f;
    float halfHeight = 0.5f;
    /// Optional triangle mesh (for Mesh type) — indices into a dedicated collision mesh later
    std::vector<Vec3> meshVertices;
    std::vector<uint32_t> meshIndices;
};

struct GltfPrefabNode {
    std::string name;
    std::string nuAsset; // extras.nu_asset
    Vec3 translation{0.0f};
    Vec3 rotationEuler{0.0f}; // radians XYZ
    Vec3 scale{1.0f};
};

struct GltfSceneData {
    std::vector<GltfMeshData> meshes;
    std::vector<GltfMaterialData> materials;
    std::vector<GltfTextureData> textures;
    std::vector<GltfCollisionNode> collisions;
    std::vector<GltfPrefabNode> prefabs;
};

[[nodiscard]] std::optional<GltfSceneData> loadGltf(const std::filesystem::path& path);

} // namespace nu
