#pragma once

#include "nu/render/vulkan_device.hpp"

#include <VkBootstrap.h>

#include <cstdint>
#include <optional>
#include <vector>

namespace nu {

class Swapchain {
public:
    void init(VulkanDevice& device, uint32_t width, uint32_t height);
    void resize(uint32_t width, uint32_t height);
    void shutdown();

    [[nodiscard]] VkSwapchainKHR handle() const { return swapchain_; }
    [[nodiscard]] VkFormat format() const { return format_; }
    [[nodiscard]] VkExtent2D extent() const { return extent_; }
    [[nodiscard]] const std::vector<VkImage>& images() const { return images_; }
    [[nodiscard]] const std::vector<VkImageView>& views() const { return views_; }

    /// Returns image index or nullopt if out of date.
    [[nodiscard]] std::optional<uint32_t> acquireNext(VkSemaphore signal, uint64_t timeout = UINT64_MAX);
    [[nodiscard]] VkResult present(VkQueue queue, uint32_t imageIndex, VkSemaphore wait);

private:
    void create(uint32_t width, uint32_t height);
    void destroy();

    VulkanDevice* device_ = nullptr;
    VkSwapchainKHR swapchain_ = VK_NULL_HANDLE;
    VkFormat format_ = VK_FORMAT_B8G8R8A8_SRGB;
    VkExtent2D extent_{};
    std::vector<VkImage> images_;
    std::vector<VkImageView> views_;
};

} // namespace nu
