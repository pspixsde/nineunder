#include "nu/render/shader.hpp"

#include "nu/core/log.hpp"
#include "nu/platform/filesystem.hpp"
#include "nu/render/vulkan_device.hpp"

#include <cstdlib>
#include <cstring>

namespace nu {

void ShaderCache::init(VulkanDevice* device) {
    device_ = device;
}

void ShaderCache::shutdown() {
    if (!device_) {
        return;
    }
    for (auto& [_, mod] : modules_) {
        if (mod.module) {
            vkDestroyShaderModule(device_->device(), mod.module, nullptr);
        }
    }
    modules_.clear();
    device_ = nullptr;
}

VkShaderModule ShaderCache::loadSpirvBytes(const uint32_t* words, size_t wordCount) {
    VkShaderModuleCreateInfo info{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
    info.codeSize = wordCount * sizeof(uint32_t);
    info.pCode = words;

    VkShaderModule module = VK_NULL_HANDLE;
    if (vkCreateShaderModule(device_->device(), &info, nullptr, &module) != VK_SUCCESS) {
        NU_LOG_ERROR("vkCreateShaderModule failed");
        return VK_NULL_HANDLE;
    }
    return module;
}

VkShaderModule ShaderCache::loadSpirvFile(const std::filesystem::path& path) {
    const std::string key = path.string();
    auto it = modules_.find(key);
    if (it != modules_.end() && it->second.module) {
        return it->second.module;
    }

    auto bytes = readBinaryFile(path);
    if (!bytes || bytes->size() < 4 || (bytes->size() % 4) != 0) {
        NU_LOG_ERROR("Invalid SPIR-V: {}", path.string());
        return VK_NULL_HANDLE;
    }

    const auto* words = reinterpret_cast<const uint32_t*>(bytes->data());
    const size_t wordCount = bytes->size() / 4;
    VkShaderModule module = loadSpirvBytes(words, wordCount);
    if (!module) {
        return VK_NULL_HANDLE;
    }

    ShaderModule entry{};
    entry.module = module;
    entry.sourcePath = path;
    entry.lastWriteNanos = fileWriteTimeNanos(path);
    modules_[key] = entry;
    NU_LOG_INFO("Loaded shader {}", path.string());
    return module;
}

bool ShaderCache::pollHotReload() {
    bool reloaded = false;
    for (auto& [key, mod] : modules_) {
        if (mod.sourcePath.empty()) {
            continue;
        }
        const uint64_t stamp = fileWriteTimeNanos(mod.sourcePath);
        if (stamp == 0 || stamp == mod.lastWriteNanos) {
            continue;
        }
        auto bytes = readBinaryFile(mod.sourcePath);
        if (!bytes || (bytes->size() % 4) != 0) {
            continue;
        }
        VkShaderModule fresh =
            loadSpirvBytes(reinterpret_cast<const uint32_t*>(bytes->data()), bytes->size() / 4);
        if (!fresh) {
            continue;
        }
        vkDestroyShaderModule(device_->device(), mod.module, nullptr);
        mod.module = fresh;
        mod.lastWriteNanos = stamp;
        reloaded = true;
        NU_LOG_INFO("Hot-reloaded shader {}", key);
    }
    return reloaded;
}

std::vector<uint32_t> compileGlslToSpirv(const std::filesystem::path& glslPath, const char* stage) {
    const fs::path outPath = glslPath.string() + ".spv";
    const std::string cmd = std::string("glslangValidator -V -S ") + stage + " \"" + glslPath.string() + "\" -o \"" +
                            outPath.string() + "\"";
    NU_LOG_DEBUG("Shader compile: {}", cmd);
    const int rc = std::system(cmd.c_str());
    if (rc != 0) {
        NU_LOG_WARN("glslangValidator failed ({}) for {}", rc, glslPath.string());
        return {};
    }
    auto bytes = readBinaryFile(outPath);
    if (!bytes || (bytes->size() % 4) != 0) {
        return {};
    }
    std::vector<uint32_t> words(bytes->size() / 4);
    std::memcpy(words.data(), bytes->data(), bytes->size());
    return words;
}

} // namespace nu
