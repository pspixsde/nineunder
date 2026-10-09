#pragma once

#include <volk.h>

#include <vk_mem_alloc.h>

#include <VkBootstrap.h>
#include <vector>

struct SDL_Window;

namespace nu {

struct AllocatedBuffer {
    VkBuffer buffer = VK_NULL_HANDLE;
    VmaAllocation allocation = VK_NULL_HANDLE;
    VmaAllocationInfo info{};
};

struct AllocatedImage {
    VkImage image = VK_NULL_HANDLE;
    VkImageView view = VK_NULL_HANDLE;
    VmaAllocation allocation = VK_NULL_HANDLE;
    VkExtent3D extent{};
    VkFormat format = VK_FORMAT_UNDEFINED;
};

class VulkanDevice {
public:
    VulkanDevice() = default;
    ~VulkanDevice();

    VulkanDevice(const VulkanDevice&) = delete;
    VulkanDevice& operator=(const VulkanDevice&) = delete;

    void init(SDL_Window* window);
    void shutdown();

    [[nodiscard]] VkInstance instance() const { return instance_; }
    [[nodiscard]] VkPhysicalDevice physicalDevice() const { return physicalDevice_; }
    [[nodiscard]] VkDevice device() const { return device_; }
    [[nodiscard]] VkQueue graphicsQueue() const { return graphicsQueue_; }
    [[nodiscard]] uint32_t graphicsQueueFamily() const { return graphicsQueueFamily_; }
    [[nodiscard]] VkSurfaceKHR surface() const { return surface_; }
    [[nodiscard]] VmaAllocator allocator() const { return allocator_; }
    [[nodiscard]] VkCommandPool commandPool() const { return commandPool_; }

    AllocatedBuffer createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage,
                                 VmaAllocationCreateFlags flags = 0);
    void destroyBuffer(AllocatedBuffer& buffer);

    AllocatedImage createImage(VkExtent3D extent, VkFormat format, VkImageUsageFlags usage,
                               VkImageAspectFlags aspect = VK_IMAGE_ASPECT_COLOR_BIT);
    void destroyImage(AllocatedImage& image);

    void immediateSubmit(void (*fn)(VkCommandBuffer, void*), void* userdata);
    template <typename Fn>
    void immediateSubmit(Fn&& fn) {
        immediateSubmit(
            [](VkCommandBuffer cmd, void* ud) {
                (*static_cast<Fn*>(ud))(cmd);
            },
            &fn);
    }

private:
    VkInstance instance_ = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT debugMessenger_ = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice_ = VK_NULL_HANDLE;
    VkDevice device_ = VK_NULL_HANDLE;
    VkQueue graphicsQueue_ = VK_NULL_HANDLE;
    uint32_t graphicsQueueFamily_ = 0;
    VkSurfaceKHR surface_ = VK_NULL_HANDLE;
    VmaAllocator allocator_ = VK_NULL_HANDLE;
    VkCommandPool commandPool_ = VK_NULL_HANDLE;
    VkFence immediateFence_ = VK_NULL_HANDLE;
};

} // namespace nu
