#include "nu/assets/gltf_loader.hpp"
#include "nu/core/log.hpp"

#include <filesystem>
#include <iostream>

int main(int argc, char** argv) {
    nu::Log::init("nu_asset_cooker");

    if (argc < 2) {
        std::cout << "Usage: nu_asset_cooker <path-to.gltf|glb>\n";
        return 1;
    }

    const std::filesystem::path input = argv[1];
    auto scene = nu::loadGltf(input);
    if (!scene) {
        NU_LOG_ERROR("Failed to load {}", input.string());
        return 2;
    }

    NU_LOG_INFO("Cooked {} mesh primitives from {}", scene->meshes.size(), input.string());
    for (const auto& mesh : scene->meshes) {
        NU_LOG_INFO("  {} — {} verts, {} indices", mesh.name, mesh.vertices.size(), mesh.indices.size());
    }
    return 0;
}
