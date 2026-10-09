#pragma once

#include "nu/core/types.hpp"

#include <vulkan/vulkan.h>

#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

namespace nu {

class VulkanDevice;

struct ShaderModule {
    VkShaderModule module = VK_NULL_HANDLE;
    std::filesystem::path sourcePath;
    uint64_t lastWriteNanos = 0;
};

/// Loads SPIR-V (.spv). Watches source paths for hot reload in dev builds.
class ShaderCache {
public:
    void init(VulkanDevice* device);
    void shutdown();

    [[nodiscard]] VkShaderModule loadSpirvFile(const std::filesystem::path& path);
    [[nodiscard]] VkShaderModule loadSpirvBytes(const uint32_t* words, size_t wordCount);

    /// Returns true if any watched shader was reloaded.
    bool pollHotReload();

private:
    VulkanDevice* device_ = nullptr;
    std::unordered_map<std::string, ShaderModule> modules_;
};

/// Compile GLSL to SPIR-V via glslangValidator if available; returns empty on failure.
[[nodiscard]] std::vector<uint32_t> compileGlslToSpirv(const std::filesystem::path& glslPath, const char* stage);

} // namespace nu
