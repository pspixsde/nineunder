#include "nu/assets/gltf_loader.hpp"

#include "nu/core/log.hpp"

#include <fastgltf/core.hpp>
#include <fastgltf/tools.hpp>
#include <fastgltf/types.hpp>

namespace nu {

std::optional<GltfSceneData> loadGltf(const std::filesystem::path& path) {
    if (!std::filesystem::exists(path)) {
        NU_LOG_ERROR("glTF not found: {}", path.string());
        return std::nullopt;
    }

    auto dataResult = fastgltf::GltfDataBuffer::FromPath(path);
    if (!dataResult) {
        NU_LOG_ERROR("Failed to read glTF buffer: {}", path.string());
        return std::nullopt;
    }

    fastgltf::Parser parser;
    constexpr auto gltfOptions = fastgltf::Options::LoadExternalBuffers;

    auto assetExpected = parser.loadGltf(dataResult.get(), path.parent_path(), gltfOptions);
    if (!assetExpected) {
        NU_LOG_ERROR("fastgltf parse failed for {}", path.string());
        return std::nullopt;
    }

    fastgltf::Asset& asset = assetExpected.get();
    GltfSceneData scene;

    for (size_t meshIndex = 0; meshIndex < asset.meshes.size(); ++meshIndex) {
        const fastgltf::Mesh& mesh = asset.meshes[meshIndex];
        for (const fastgltf::Primitive& prim : mesh.primitives) {
            GltfMeshData out;
            out.name = mesh.name.empty() ? ("mesh_" + std::to_string(meshIndex)) : std::string(mesh.name);

            const auto* positionIt = prim.findAttribute("POSITION");
            if (positionIt == nullptr) {
                continue;
            }

            const fastgltf::Accessor& posAccessor = asset.accessors[positionIt->accessorIndex];
            out.vertices.resize(posAccessor.count);

            fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec3>(
                asset, posAccessor, [&](const fastgltf::math::fvec3& v, std::size_t idx) {
                    out.vertices[idx].position = {v.x(), v.y(), v.z()};
                    out.vertices[idx].normal = {0.0f, 1.0f, 0.0f};
                    out.vertices[idx].uv = {0.0f, 0.0f};
                    out.vertices[idx].color = {1.0f, 1.0f, 1.0f, 1.0f};
                });

            if (const auto* normalIt = prim.findAttribute("NORMAL"); normalIt != nullptr) {
                const fastgltf::Accessor& nAcc = asset.accessors[normalIt->accessorIndex];
                fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec3>(
                    asset, nAcc, [&](const fastgltf::math::fvec3& v, std::size_t idx) {
                        out.vertices[idx].normal = {v.x(), v.y(), v.z()};
                    });
            }

            if (const auto* uvIt = prim.findAttribute("TEXCOORD_0"); uvIt != nullptr) {
                const fastgltf::Accessor& uvAcc = asset.accessors[uvIt->accessorIndex];
                fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec2>(
                    asset, uvAcc, [&](const fastgltf::math::fvec2& v, std::size_t idx) {
                        out.vertices[idx].uv = {v.x(), v.y()};
                    });
            }

            if (prim.indicesAccessor.has_value()) {
                const fastgltf::Accessor& iAcc = asset.accessors[*prim.indicesAccessor];
                out.indices.reserve(iAcc.count);
                fastgltf::iterateAccessor<std::uint32_t>(asset, iAcc,
                                                         [&](std::uint32_t idx) { out.indices.push_back(idx); });
            } else {
                out.indices.resize(out.vertices.size());
                for (uint32_t i = 0; i < static_cast<uint32_t>(out.indices.size()); ++i) {
                    out.indices[i] = i;
                }
            }

            if (prim.materialIndex.has_value() && *prim.materialIndex < asset.materials.size()) {
                const fastgltf::Material& mat = asset.materials[*prim.materialIndex];
                const auto& base = mat.pbrData.baseColorFactor;
                out.material.baseColor = {base[0], base[1], base[2], base[3]};
                out.material.metallic = mat.pbrData.metallicFactor;
                out.material.roughness = mat.pbrData.roughnessFactor;
            }

            scene.meshes.push_back(std::move(out));
        }
    }

    NU_LOG_INFO("Loaded glTF {} ({} primitives)", path.string(), scene.meshes.size());
    return scene;
}

} // namespace nu
