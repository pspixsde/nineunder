#include "nu/assets/texture_loader.hpp"

#include "nu/core/log.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <cstring>

namespace nu {

std::optional<CpuTexture> loadTextureCpu(const std::filesystem::path& path) {
    int w = 0;
    int h = 0;
    int c = 0;
    stbi_uc* data = stbi_load(path.string().c_str(), &w, &h, &c, 4);
    if (!data) {
        NU_LOG_ERROR("stb_image failed: {}", path.string());
        return std::nullopt;
    }
    CpuTexture tex;
    tex.width = w;
    tex.height = h;
    tex.channels = 4;
    tex.pixels.resize(static_cast<size_t>(w * h * 4));
    std::memcpy(tex.pixels.data(), data, tex.pixels.size());
    stbi_image_free(data);
    return tex;
}

AllocatedImage uploadTexture(VulkanDevice& device, const CpuTexture& cpu) {
    const VkDeviceSize size = cpu.pixels.size();
    AllocatedBuffer staging =
        device.createBuffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_AUTO,
                            VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT);
    std::memcpy(staging.info.pMappedData, cpu.pixels.data(), static_cast<size_t>(size));

    AllocatedImage image =
        device.createImage({static_cast<uint32_t>(cpu.width), static_cast<uint32_t>(cpu.height), 1},
                           VK_FORMAT_R8G8B8A8_SRGB,
                           VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);

    device.immediateSubmit([&](VkCommandBuffer cmd) {
        VkImageMemoryBarrier2 barrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2};
        barrier.srcStageMask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
        barrier.dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
        barrier.dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
        barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.image = image.image;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.layerCount = 1;
        VkDependencyInfo dep{VK_STRUCTURE_TYPE_DEPENDENCY_INFO};
        dep.imageMemoryBarrierCount = 1;
        dep.pImageMemoryBarriers = &barrier;
        vkCmdPipelineBarrier2(cmd, &dep);

        VkBufferImageCopy copy{};
        copy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        copy.imageSubresource.layerCount = 1;
        copy.imageExtent = {static_cast<uint32_t>(cpu.width), static_cast<uint32_t>(cpu.height), 1};
        vkCmdCopyBufferToImage(cmd, staging.buffer, image.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy);

        barrier.srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
        barrier.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
        barrier.dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
        barrier.dstAccessMask = VK_ACCESS_2_SHADER_SAMPLED_READ_BIT;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        vkCmdPipelineBarrier2(cmd, &dep);
    });

    device.destroyBuffer(staging);
    return image;
}

} // namespace nu
