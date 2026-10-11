#pragma once

#include "nu/assets/gltf_loader.hpp"
#include "nu/render/mesh.hpp"
#include "nu/render/renderer.hpp"

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace nu {

struct ModelLodMesh {
    GpuMesh mesh{};
    MaterialGpu material{};
    VkDescriptorSet materialSet = VK_NULL_HANDLE;
    int lod = 0;
    std::string name;
};

struct ModelCollision {
    GltfCollisionType type = GltfCollisionType::None;
    Vec3 translation{0.0f};
    Vec3 halfExtents{0.5f};
    float radius = 0.4f;
    float halfHeight = 0.5f;
    std::string name;
};

/// GPU-resident static model loaded from glTF/GLB.
class ModelAsset {
public:
    ModelAsset() = default;
    ~ModelAsset();

    ModelAsset(const ModelAsset&) = delete;
    ModelAsset& operator=(const ModelAsset&) = delete;
    ModelAsset(ModelAsset&& other) noexcept;
    ModelAsset& operator=(ModelAsset&& other) noexcept;

    [[nodiscard]] static std::optional<ModelAsset> load(Renderer& renderer, const std::filesystem::path& path);

    void destroy(Renderer& renderer);

    [[nodiscard]] const std::vector<ModelLodMesh>& meshes() const { return meshes_; }
    [[nodiscard]] const std::vector<ModelCollision>& collisions() const { return collisions_; }
    [[nodiscard]] const std::vector<GltfPrefabNode>& prefabs() const { return prefabs_; }
    [[nodiscard]] const std::filesystem::path& sourcePath() const { return sourcePath_; }
    [[nodiscard]] bool valid() const { return !meshes_.empty(); }

    /// Append draw items for LOD0 (or best available) at the given transform.
    void appendDraws(std::vector<DrawItem>& draws, const Mat4& transform, int lod = 0) const;

private:
    std::vector<ModelLodMesh> meshes_;
    std::vector<AllocatedImage> images_;
    std::vector<ModelCollision> collisions_;
    std::vector<GltfPrefabNode> prefabs_;
    std::filesystem::path sourcePath_;
    Renderer* renderer_ = nullptr;
};

} // namespace nu
