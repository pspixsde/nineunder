#include "nu/assets/model_asset.hpp"

#include "nu/assets/texture_loader.hpp"
#include "nu/core/log.hpp"

#include <algorithm>
#include <utility>

namespace nu {

ModelAsset::~ModelAsset() {
    if (renderer_) {
        destroy(*renderer_);
    }
}

ModelAsset::ModelAsset(ModelAsset&& other) noexcept
    : meshes_(std::move(other.meshes_)), images_(std::move(other.images_)),
      collisions_(std::move(other.collisions_)), prefabs_(std::move(other.prefabs_)),
      sourcePath_(std::move(other.sourcePath_)), renderer_(other.renderer_) {
    other.renderer_ = nullptr;
}

ModelAsset& ModelAsset::operator=(ModelAsset&& other) noexcept {
    if (this != &other) {
        if (renderer_) {
            destroy(*renderer_);
        }
        meshes_ = std::move(other.meshes_);
        images_ = std::move(other.images_);
        collisions_ = std::move(other.collisions_);
        prefabs_ = std::move(other.prefabs_);
        sourcePath_ = std::move(other.sourcePath_);
        renderer_ = other.renderer_;
        other.renderer_ = nullptr;
    }
    return *this;
}

void ModelAsset::destroy(Renderer& renderer) {
    for (ModelLodMesh& m : meshes_) {
        if (m.materialSet && m.materialSet != renderer.defaultMaterialSet()) {
            vkFreeDescriptorSets(renderer.device().device(), renderer.materialDescriptorPool(), 1, &m.materialSet);
        }
        renderer.meshes().destroy(m.mesh);
        m.materialSet = VK_NULL_HANDLE;
    }
    meshes_.clear();
    for (AllocatedImage& img : images_) {
        renderer.device().destroyImage(img);
    }
    images_.clear();
    collisions_.clear();
    prefabs_.clear();
    renderer_ = nullptr;
}

std::optional<ModelAsset> ModelAsset::load(Renderer& renderer, const std::filesystem::path& path) {
    auto scene = loadGltf(path);
    if (!scene) {
        return std::nullopt;
    }

    ModelAsset model;
    model.renderer_ = &renderer;
    model.sourcePath_ = path;
    model.prefabs_ = scene->prefabs;

    for (const GltfCollisionNode& c : scene->collisions) {
        ModelCollision mc;
        mc.type = c.type;
        mc.translation = c.translation;
        mc.halfExtents = c.halfExtents;
        mc.radius = c.radius;
        mc.halfHeight = c.halfHeight;
        mc.name = c.name;
        model.collisions_.push_back(mc);
    }

    // Upload textures
    std::vector<VkImageView> views;
    views.reserve(scene->textures.size());
    for (const GltfTextureData& tex : scene->textures) {
        CpuTexture cpu;
        cpu.width = tex.width;
        cpu.height = tex.height;
        cpu.channels = tex.channels;
        cpu.pixels = tex.pixels;
        AllocatedImage img = uploadTexture(renderer.device(), cpu, tex.srgb);
        views.push_back(img.view);
        model.images_.push_back(std::move(img));
    }

    auto viewOrNull = [&](int index) -> VkImageView {
        if (index < 0 || static_cast<size_t>(index) >= views.size()) {
            return VK_NULL_HANDLE;
        }
        return views[static_cast<size_t>(index)];
    };

    for (const GltfMeshData& meshData : scene->meshes) {
        ModelLodMesh lod;
        lod.name = meshData.name;
        lod.lod = meshData.lod;
        lod.mesh = renderer.meshes().create(meshData.vertices, meshData.indices);

        MaterialGpu mat{};
        int baseIdx = -1;
        int normalIdx = -1;
        int ormIdx = -1;
        int emissiveIdx = -1;
        if (meshData.materialIndex >= 0 &&
            static_cast<size_t>(meshData.materialIndex) < scene->materials.size()) {
            const GltfMaterialData& md = scene->materials[static_cast<size_t>(meshData.materialIndex)];
            mat = md.gpu;
            baseIdx = md.baseColorTexture;
            normalIdx = md.normalTexture;
            ormIdx = md.ormTexture;
            emissiveIdx = md.emissiveTexture;
        }
        lod.material = mat;
        lod.materialSet = renderer.createMaterialSet(viewOrNull(baseIdx), viewOrNull(normalIdx), viewOrNull(ormIdx),
                                                     viewOrNull(emissiveIdx));
        model.meshes_.push_back(std::move(lod));
    }

    if (model.meshes_.empty()) {
        NU_LOG_ERROR("ModelAsset has no meshes: {}", path.string());
        model.destroy(renderer);
        return std::nullopt;
    }

    NU_LOG_INFO("ModelAsset ready: {} ({} meshes)", path.string(), model.meshes_.size());
    return model;
}

void ModelAsset::appendDraws(std::vector<DrawItem>& draws, const Mat4& transform, int lod) const {
    bool any = false;
    for (const ModelLodMesh& m : meshes_) {
        if (m.lod == lod) {
            DrawItem item;
            item.mesh = &m.mesh;
            item.transform = transform;
            item.material = m.material;
            item.materialSet = m.materialSet;
            draws.push_back(item);
            any = true;
        }
    }
    if (!any) {
        // Fallback to whatever is available (prefer lowest lod number)
        int best = 1000;
        for (const ModelLodMesh& m : meshes_) {
            best = std::min(best, m.lod);
        }
        for (const ModelLodMesh& m : meshes_) {
            if (m.lod != best) {
                continue;
            }
            DrawItem item;
            item.mesh = &m.mesh;
            item.transform = transform;
            item.material = m.material;
            item.materialSet = m.materialSet;
            draws.push_back(item);
        }
    }
}

} // namespace nu
