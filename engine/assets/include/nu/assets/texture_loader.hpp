#pragma once

#include "nu/render/vulkan_device.hpp"

#include <cstdint>
#include <filesystem>
#include <optional>
#include <vector>

namespace nu {

struct CpuTexture {
    int width = 0;
    int height = 0;
    int channels = 0;
    std::vector<uint8_t> pixels;
};

[[nodiscard]] std::optional<CpuTexture> loadTextureCpu(const std::filesystem::path& path);
[[nodiscard]] AllocatedImage uploadTexture(VulkanDevice& device, const CpuTexture& cpu,
                                           bool srgb = true);

} // namespace nu
