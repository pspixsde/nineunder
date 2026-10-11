#include "nu/assets/gltf_loader.hpp"
#include "nu/core/log.hpp"

#include <meshoptimizer.h>

#include <ktx.h>

#ifndef VK_FORMAT_R8G8B8A8_UNORM
#define VK_FORMAT_R8G8B8A8_UNORM 37
#endif
#ifndef VK_FORMAT_R8G8B8A8_SRGB
#define VK_FORMAT_R8G8B8A8_SRGB 43
#endif

#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace {

struct Options {
    std::filesystem::path input;
    bool validate = false;
    bool optimize = true;
    std::filesystem::path cookOut;
    int trisMax = 50000;
};

void printUsage() {
    std::cout << "Usage: nu_asset_cooker <path-to.gltf|glb> [--validate] [--no-optimize] [--cook-out <dir>]\n"
                 "                       [--tris-max N]\n";
}

bool parseArgs(int argc, char** argv, Options& opt) {
    if (argc < 2) {
        return false;
    }
    opt.input = argv[1];
    for (int i = 2; i < argc; ++i) {
        const std::string a = argv[i];
        if (a == "--validate") {
            opt.validate = true;
        } else if (a == "--no-optimize") {
            opt.optimize = false;
        } else if (a == "--cook-out" && i + 1 < argc) {
            opt.cookOut = argv[++i];
        } else if (a == "--tris-max" && i + 1 < argc) {
            opt.trisMax = std::atoi(argv[++i]);
        } else {
            std::cerr << "Unknown arg: " << a << "\n";
            return false;
        }
    }
    return true;
}

std::size_t optimizeMesh(nu::GltfMeshData& mesh) {
    if (mesh.indices.empty() || mesh.vertices.empty()) {
        return 0;
    }
    const size_t vertexCount = mesh.vertices.size();
    const size_t indexCount = mesh.indices.size();

    std::vector<uint32_t> remap(vertexCount);
    const size_t unique = meshopt_generateVertexRemap(remap.data(), mesh.indices.data(), indexCount,
                                                      mesh.vertices.data(), vertexCount, sizeof(nu::Vertex));

    std::vector<nu::Vertex> newVerts(unique);
    std::vector<uint32_t> newIndices(indexCount);
    meshopt_remapIndexBuffer(newIndices.data(), mesh.indices.data(), indexCount, remap.data());
    meshopt_remapVertexBuffer(newVerts.data(), mesh.vertices.data(), vertexCount, sizeof(nu::Vertex), remap.data());
    meshopt_optimizeVertexCache(newIndices.data(), newIndices.data(), indexCount, unique);
    meshopt_optimizeOverdraw(newIndices.data(), newIndices.data(), indexCount, &newVerts[0].position.x, unique,
                             sizeof(nu::Vertex), 1.05f);
    meshopt_optimizeVertexFetch(newVerts.data(), newIndices.data(), indexCount, newVerts.data(), unique,
                                sizeof(nu::Vertex));

    mesh.vertices = std::move(newVerts);
    mesh.indices = std::move(newIndices);
    return unique;
}

} // namespace

int main(int argc, char** argv) {
    nu::Log::init("nu_asset_cooker");

    Options opt;
    if (!parseArgs(argc, argv, opt)) {
        printUsage();
        return 1;
    }

    auto scene = nu::loadGltf(opt.input);
    if (!scene) {
        NU_LOG_ERROR("Failed to load {}", opt.input.string());
        return 2;
    }

    std::size_t totalTris = 0;
    for (auto& mesh : scene->meshes) {
        if (opt.optimize) {
            const std::size_t unique = optimizeMesh(mesh);
            NU_LOG_INFO("  optimized {} → {} verts, {} indices", mesh.name, unique, mesh.indices.size());
        } else {
            NU_LOG_INFO("  {} — {} verts, {} indices", mesh.name, mesh.vertices.size(), mesh.indices.size());
        }
        totalTris += mesh.indices.size() / 3;
    }

    NU_LOG_INFO("Cooked {} mesh primitives from {} ({} tris, {} mats, {} tex)", scene->meshes.size(),
                opt.input.string(), totalTris, scene->materials.size(), scene->textures.size());

    if (opt.validate) {
        bool ok = true;
        if (scene->meshes.empty()) {
            NU_LOG_ERROR("validate: no meshes");
            ok = false;
        }
        if (static_cast<int>(totalTris) > opt.trisMax) {
            NU_LOG_ERROR("validate: tris {} exceeds max {}", totalTris, opt.trisMax);
            ok = false;
        }
        for (const auto& mesh : scene->meshes) {
            if (mesh.indices.size() % 3 != 0) {
                NU_LOG_ERROR("validate: {} index count not divisible by 3", mesh.name);
                ok = false;
            }
        }
        if (!ok) {
            return 3;
        }
        NU_LOG_INFO("validate: OK");
    }

    if (!opt.cookOut.empty()) {
        std::filesystem::create_directories(opt.cookOut);
        const auto texDir = opt.cookOut / "textures";
        std::filesystem::create_directories(texDir);

        for (size_t i = 0; i < scene->textures.size(); ++i) {
            const auto& tex = scene->textures[i];
            if (tex.width <= 0 || tex.height <= 0 || tex.pixels.empty()) {
                continue;
            }
            ktxTexture2* ktex = nullptr;
            ktxTextureCreateInfo createInfo{};
            createInfo.vkFormat = tex.srgb ? VK_FORMAT_R8G8B8A8_SRGB : VK_FORMAT_R8G8B8A8_UNORM;
            createInfo.baseWidth = static_cast<ktx_uint32_t>(tex.width);
            createInfo.baseHeight = static_cast<ktx_uint32_t>(tex.height);
            createInfo.baseDepth = 1;
            createInfo.numDimensions = 2;
            createInfo.numLevels = 1;
            createInfo.numLayers = 1;
            createInfo.numFaces = 1;
            createInfo.isArray = KTX_FALSE;
            createInfo.generateMipmaps = KTX_FALSE;
            if (ktxTexture2_Create(&createInfo, KTX_TEXTURE_CREATE_ALLOC_STORAGE, &ktex) != KTX_SUCCESS) {
                NU_LOG_WARN("KTX2 create failed for texture {}", i);
                continue;
            }
            std::memcpy(ktxTexture_GetData(ktxTexture(ktex)), tex.pixels.data(), tex.pixels.size());
            const auto outPath = texDir / (tex.name.empty() ? ("tex_" + std::to_string(i) + ".ktx2") : (tex.name + ".ktx2"));
            if (ktxTexture_WriteToNamedFile(ktxTexture(ktex), outPath.string().c_str()) != KTX_SUCCESS) {
                NU_LOG_WARN("KTX2 write failed: {}", outPath.string());
            } else {
                NU_LOG_INFO("Wrote {}", outPath.string());
            }
            ktxTexture_Destroy(ktxTexture(ktex));
        }

        const auto manifest = opt.cookOut / (opt.input.stem().string() + ".cook.txt");
        std::ofstream out(manifest);
        if (out) {
            out << "source=" << opt.input.string() << "\n";
            out << "meshes=" << scene->meshes.size() << "\n";
            out << "tris=" << totalTris << "\n";
            out << "textures=" << scene->textures.size() << "\n";
            out << "optimized=" << (opt.optimize ? "1" : "0") << "\n";
            NU_LOG_INFO("Wrote cook manifest {}", manifest.string());
        }
    }

    return 0;
}
