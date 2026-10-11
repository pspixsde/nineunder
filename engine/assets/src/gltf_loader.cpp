#include "nu/assets/gltf_loader.hpp"

#include "nu/core/log.hpp"

#include <fastgltf/core.hpp>
#include <fastgltf/tools.hpp>
#include <fastgltf/types.hpp>

#include <stb_image.h>

#include <cctype>
#include <cstring>
#include <string_view>

namespace nu {
namespace {

int parseLodSuffix(std::string_view name) {
    const auto pos = name.rfind("_LOD");
    if (pos == std::string_view::npos || pos + 4 >= name.size()) {
        return 0;
    }
    int lod = 0;
    for (size_t i = pos + 4; i < name.size(); ++i) {
        if (!std::isdigit(static_cast<unsigned char>(name[i]))) {
            return 0;
        }
        lod = lod * 10 + (name[i] - '0');
    }
    return lod;
}

int imageIndexFromTexture(const fastgltf::Asset& asset, std::size_t textureIndex) {
    if (textureIndex >= asset.textures.size()) {
        return -1;
    }
    const auto& tex = asset.textures[textureIndex];
    if (!tex.imageIndex.has_value()) {
        return -1;
    }
    return static_cast<int>(*tex.imageIndex);
}

bool decodeImage(const std::byte* bytes, std::size_t size, GltfTextureData& out) {
    int w = 0;
    int h = 0;
    int c = 0;
    stbi_uc* data =
        stbi_load_from_memory(reinterpret_cast<const stbi_uc*>(bytes), static_cast<int>(size), &w, &h, &c, 4);
    if (!data) {
        return false;
    }
    out.width = w;
    out.height = h;
    out.channels = 4;
    out.pixels.resize(static_cast<size_t>(w * h * 4));
    std::memcpy(out.pixels.data(), data, out.pixels.size());
    stbi_image_free(data);
    return true;
}

bool loadImagePixels(fastgltf::Asset& asset, std::size_t imageIndex, GltfTextureData& out) {
    if (imageIndex >= asset.images.size()) {
        return false;
    }
    const fastgltf::Image& image = asset.images[imageIndex];
    out.name = image.name.empty() ? ("tex_" + std::to_string(imageIndex)) : std::string(image.name);

    return std::visit(
        fastgltf::visitor{
            [&](const fastgltf::sources::Array& arr) {
                return decodeImage(arr.bytes.data(), arr.bytes.size(), out);
            },
            [&](const fastgltf::sources::Vector& vec) {
                return decodeImage(vec.bytes.data(), vec.bytes.size(), out);
            },
            [&](const fastgltf::sources::ByteView& view) {
                return decodeImage(view.bytes.data(), view.bytes.size(), out);
            },
            [&](const fastgltf::sources::BufferView& view) {
                if (view.bufferViewIndex >= asset.bufferViews.size()) {
                    return false;
                }
                const fastgltf::BufferView& bv = asset.bufferViews[view.bufferViewIndex];
                if (bv.bufferIndex >= asset.buffers.size()) {
                    return false;
                }
                const fastgltf::Buffer& buffer = asset.buffers[bv.bufferIndex];
                return std::visit(fastgltf::visitor{
                                      [&](const fastgltf::sources::Array& arr) {
                                          if (bv.byteOffset + bv.byteLength > arr.bytes.size()) {
                                              return false;
                                          }
                                          return decodeImage(arr.bytes.data() + bv.byteOffset, bv.byteLength, out);
                                      },
                                      [&](const fastgltf::sources::Vector& vec) {
                                          if (bv.byteOffset + bv.byteLength > vec.bytes.size()) {
                                              return false;
                                          }
                                          return decodeImage(vec.bytes.data() + bv.byteOffset, bv.byteLength, out);
                                      },
                                      [&](const fastgltf::sources::ByteView& byteView) {
                                          if (bv.byteOffset + bv.byteLength > byteView.bytes.size()) {
                                              return false;
                                          }
                                          return decodeImage(byteView.bytes.data() + bv.byteOffset, bv.byteLength, out);
                                      },
                                      [](const auto&) { return false; },
                                  },
                                  buffer.data);
            },
            [&](const fastgltf::sources::URI&) {
                NU_LOG_WARN("glTF image URI without embedded bytes: {}", out.name);
                return false;
            },
            [](const auto&) { return false; },
        },
        image.data);
}

} // namespace

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
    constexpr auto gltfOptions = fastgltf::Options::LoadExternalBuffers | fastgltf::Options::LoadExternalImages |
                                 fastgltf::Options::GenerateMeshIndices;

    auto assetExpected = parser.loadGltf(dataResult.get(), path.parent_path(), gltfOptions);
    if (!assetExpected) {
        NU_LOG_ERROR("fastgltf parse failed for {}", path.string());
        return std::nullopt;
    }

    fastgltf::Asset& asset = assetExpected.get();
    GltfSceneData scene;

    scene.textures.resize(asset.images.size());
    for (size_t i = 0; i < asset.images.size(); ++i) {
        GltfTextureData tex;
        if (!loadImagePixels(asset, i, tex)) {
            tex.width = 1;
            tex.height = 1;
            tex.channels = 4;
            tex.pixels = {255, 0, 255, 255};
            NU_LOG_WARN("Failed to decode image {} — using placeholder", i);
        }
        scene.textures[i] = std::move(tex);
    }

    for (size_t mi = 0; mi < asset.materials.size(); ++mi) {
        const fastgltf::Material& mat = asset.materials[mi];
        GltfMaterialData out;
        out.name = mat.name.empty() ? ("mat_" + std::to_string(mi)) : std::string(mat.name);
        const auto& base = mat.pbrData.baseColorFactor;
        out.gpu.baseColor = {base[0], base[1], base[2], base[3]};
        out.gpu.metallic = mat.pbrData.metallicFactor;
        out.gpu.roughness = mat.pbrData.roughnessFactor;
        out.gpu.emissive = {mat.emissiveFactor[0], mat.emissiveFactor[1], mat.emissiveFactor[2], 1.0f};

        if (mat.pbrData.baseColorTexture.has_value()) {
            out.baseColorTexture = imageIndexFromTexture(asset, mat.pbrData.baseColorTexture->textureIndex);
        }
        if (mat.normalTexture.has_value()) {
            out.normalTexture = imageIndexFromTexture(asset, mat.normalTexture->textureIndex);
        }
        if (mat.pbrData.metallicRoughnessTexture.has_value()) {
            out.ormTexture = imageIndexFromTexture(asset, mat.pbrData.metallicRoughnessTexture->textureIndex);
        }
        if (mat.emissiveTexture.has_value()) {
            out.emissiveTexture = imageIndexFromTexture(asset, mat.emissiveTexture->textureIndex);
        }

        out.gpu.useBaseColorMap = out.baseColorTexture >= 0 ? 1.0f : 0.0f;
        out.gpu.useNormalMap = out.normalTexture >= 0 ? 1.0f : 0.0f;
        if (out.ormTexture >= 0) {
            scene.textures[static_cast<size_t>(out.ormTexture)].srgb = false;
        }
        if (out.normalTexture >= 0) {
            scene.textures[static_cast<size_t>(out.normalTexture)].srgb = false;
        }
        scene.materials.push_back(std::move(out));
    }

    for (size_t meshIndex = 0; meshIndex < asset.meshes.size(); ++meshIndex) {
        const fastgltf::Mesh& mesh = asset.meshes[meshIndex];
        const std::string meshName = mesh.name.empty() ? ("mesh_" + std::to_string(meshIndex)) : std::string(mesh.name);
        for (const fastgltf::Primitive& prim : mesh.primitives) {
            GltfMeshData out;
            out.name = meshName;
            out.lod = parseLodSuffix(meshName);

            const auto positionIt = prim.findAttribute("POSITION");
            if (positionIt == prim.attributes.cend()) {
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
                    out.vertices[idx].tangent = {1.0f, 0.0f, 0.0f, 1.0f};
                });

            if (const auto normalIt = prim.findAttribute("NORMAL"); normalIt != prim.attributes.cend()) {
                const fastgltf::Accessor& nAcc = asset.accessors[normalIt->accessorIndex];
                fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec3>(
                    asset, nAcc, [&](const fastgltf::math::fvec3& v, std::size_t idx) {
                        out.vertices[idx].normal = {v.x(), v.y(), v.z()};
                    });
            }

            if (const auto uvIt = prim.findAttribute("TEXCOORD_0"); uvIt != prim.attributes.cend()) {
                const fastgltf::Accessor& uvAcc = asset.accessors[uvIt->accessorIndex];
                fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec2>(
                    asset, uvAcc, [&](const fastgltf::math::fvec2& v, std::size_t idx) {
                        out.vertices[idx].uv = {v.x(), v.y()};
                    });
            }

            if (const auto colIt = prim.findAttribute("COLOR_0"); colIt != prim.attributes.cend()) {
                const fastgltf::Accessor& cAcc = asset.accessors[colIt->accessorIndex];
                if (cAcc.type == fastgltf::AccessorType::Vec4) {
                    fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec4>(
                        asset, cAcc, [&](const fastgltf::math::fvec4& v, std::size_t idx) {
                            out.vertices[idx].color = {v.x(), v.y(), v.z(), v.w()};
                        });
                }
            }

            if (const auto tanIt = prim.findAttribute("TANGENT"); tanIt != prim.attributes.cend()) {
                const fastgltf::Accessor& tAcc = asset.accessors[tanIt->accessorIndex];
                fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec4>(
                    asset, tAcc, [&](const fastgltf::math::fvec4& v, std::size_t idx) {
                        out.vertices[idx].tangent = {v.x(), v.y(), v.z(), v.w()};
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

            if (prim.materialIndex.has_value()) {
                out.materialIndex = static_cast<int>(*prim.materialIndex);
            }
            scene.meshes.push_back(std::move(out));
        }
    }

    for (size_t ni = 0; ni < asset.nodes.size(); ++ni) {
        const fastgltf::Node& node = asset.nodes[ni];
        const std::string nodeName = node.name.empty() ? ("node_" + std::to_string(ni)) : std::string(node.name);

        Vec3 translation{0.0f};
        Vec3 scale{1.0f};
        if (auto* trs = std::get_if<fastgltf::TRS>(&node.transform)) {
            translation = {trs->translation.x(), trs->translation.y(), trs->translation.z()};
            scale = {trs->scale.x(), trs->scale.y(), trs->scale.z()};
        }

        if (nodeName.rfind("COL_", 0) == 0) {
            GltfCollisionNode col;
            col.name = nodeName;
            col.translation = translation;
            if (nodeName.find("capsule") != std::string::npos) {
                col.type = GltfCollisionType::Capsule;
                col.radius = std::max(std::abs(scale.x), std::abs(scale.z)) * 0.5f;
                col.halfHeight = std::max(std::abs(scale.y) * 0.5f - col.radius, 0.01f);
            } else if (nodeName.find("mesh") != std::string::npos) {
                col.type = GltfCollisionType::Mesh;
            } else {
                col.type = GltfCollisionType::Box;
                col.halfExtents = {std::abs(scale.x) * 0.5f, std::abs(scale.y) * 0.5f, std::abs(scale.z) * 0.5f};
                if (col.halfExtents.x < 1e-4f && col.halfExtents.y < 1e-4f && col.halfExtents.z < 1e-4f) {
                    col.halfExtents = {0.5f, 0.5f, 0.5f};
                }
            }
            // Empty COL_box nodes often use identity scale; prefer half-extent from translation-sized default
            if (col.type == GltfCollisionType::Box && scale.x == 1.0f && scale.y == 1.0f && scale.z == 1.0f) {
                col.halfExtents = {0.5f, 0.5f, 0.5f};
            }
            scene.collisions.push_back(col);
        }

        if (nodeName.rfind("SPAWN_", 0) == 0) {
            GltfPrefabNode prefab;
            prefab.name = nodeName;
            prefab.nuAsset = nodeName.substr(6);
            prefab.translation = translation;
            prefab.scale = scale;
            scene.prefabs.push_back(prefab);
        }
    }

    NU_LOG_INFO("Loaded glTF {} ({} prims, {} mats, {} tex, {} col)", path.string(), scene.meshes.size(),
                scene.materials.size(), scene.textures.size(), scene.collisions.size());
    return scene;
}

} // namespace nu
