#include "nu/render/vulkan_device.hpp"

#include "nu/core/log.hpp"

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#include <stdexcept>

namespace nu {

VulkanDevice::~VulkanDevice() {
    shutdown();
}

void VulkanDevice::init(SDL_Window* window) {
    volkInitialize();

    vkb::InstanceBuilder builder;
    auto instRet =
        builder.set_app_name("Nine Under")
            .set_engine_name("nu_engine")
            .require_api_version(1, 3, 0)
#if defined(NU_VALIDATION)
            .request_validation_layers(true)
            .use_default_debug_messenger()
#endif
            .enable_extension(VK_KHR_SURFACE_EXTENSION_NAME)
            .build();

    if (!instRet) {
        throw std::runtime_error("Failed to create Vulkan instance: " + instRet.error().message());
    }

    vkb::Instance vkbInst = instRet.value();
    instance_ = vkbInst.instance;
    debugMessenger_ = vkbInst.debug_messenger;
    volkLoadInstance(instance_);

    if (!SDL_Vulkan_CreateSurface(window, instance_, nullptr, &surface_)) {
        throw std::runtime_error(std::string("SDL_Vulkan_CreateSurface failed: ") + SDL_GetError());
    }

    VkPhysicalDeviceVulkan13Features features13{};
    features13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
    features13.dynamicRendering = VK_TRUE;
    features13.synchronization2 = VK_TRUE;

    VkPhysicalDeviceVulkan12Features features12{};
    features12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
    features12.bufferDeviceAddress = VK_TRUE;
    features12.descriptorIndexing = VK_TRUE;
    features12.runtimeDescriptorArray = VK_TRUE;
    features12.descriptorBindingPartiallyBound = VK_TRUE;
    features12.descriptorBindingVariableDescriptorCount = VK_TRUE;
    features12.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;
    features12.pNext = &features13;

    VkPhysicalDeviceFeatures features{};
    features.samplerAnisotropy = VK_TRUE;
    features.fillModeNonSolid = VK_TRUE;

    vkb::PhysicalDeviceSelector selector{vkbInst};
    auto physRet = selector.set_surface(surface_)
                       .set_minimum_version(1, 3)
                       .set_required_features(features)
                       .set_required_features_12(features12)
                       .select();
    if (!physRet) {
        // Retry without optional 1.2 bindless bits some drivers reject in combination.
        features12 = {};
        features12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
        features12.bufferDeviceAddress = VK_TRUE;
        features12.descriptorIndexing = VK_TRUE;
        features12.pNext = &features13;
        physRet = selector.set_surface(surface_)
                      .set_minimum_version(1, 3)
                      .set_required_features(features)
                      .set_required_features_12(features12)
                      .select();
    }
    if (!physRet) {
        throw std::runtime_error("Failed to select physical device: " + physRet.error().message());
    }

    vkb::PhysicalDevice physical = physRet.value();
    physicalDevice_ = physical.physical_device;

    vkb::DeviceBuilder deviceBuilder{physical};
    auto devRet = deviceBuilder.build();
    if (!devRet) {
        throw std::runtime_error("Failed to create device: " + devRet.error().message());
    }

    vkb::Device vkbDevice = devRet.value();
    device_ = vkbDevice.device;
    volkLoadDevice(device_);

    graphicsQueue_ = vkbDevice.get_queue(vkb::QueueType::graphics).value();
    graphicsQueueFamily_ = vkbDevice.get_queue_index(vkb::QueueType::graphics).value();

    VmaAllocatorCreateInfo allocatorInfo{};
    allocatorInfo.physicalDevice = physicalDevice_;
    allocatorInfo.device = device_;
    allocatorInfo.instance = instance_;
    allocatorInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
    allocatorInfo.vulkanApiVersion = VK_API_VERSION_1_3;

    VmaVulkanFunctions vmaFns{};
    vmaFns.vkGetInstanceProcAddr = vkGetInstanceProcAddr;
    vmaFns.vkGetDeviceProcAddr = vkGetDeviceProcAddr;
    allocatorInfo.pVulkanFunctions = &vmaFns;

    if (vmaCreateAllocator(&allocatorInfo, &allocator_) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create VMA allocator");
    }

    VkCommandPoolCreateInfo poolInfo{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = graphicsQueueFamily_;
    if (vkCreateCommandPool(device_, &poolInfo, nullptr, &commandPool_) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create command pool");
    }

    VkFenceCreateInfo fenceInfo{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
    vkCreateFence(device_, &fenceInfo, nullptr, &immediateFence_);

    NU_LOG_INFO("Vulkan device ready: {}", physical.properties.deviceName);
}

void VulkanDevice::shutdown() {
    if (!device_) {
        return;
    }

    vkDeviceWaitIdle(device_);

    if (immediateFence_) {
        vkDestroyFence(device_, immediateFence_, nullptr);
        immediateFence_ = VK_NULL_HANDLE;
    }
    if (commandPool_) {
        vkDestroyCommandPool(device_, commandPool_, nullptr);
        commandPool_ = VK_NULL_HANDLE;
    }
    if (allocator_) {
        vmaDestroyAllocator(allocator_);
        allocator_ = VK_NULL_HANDLE;
    }
    if (device_) {
        vkDestroyDevice(device_, nullptr);
        device_ = VK_NULL_HANDLE;
    }
    if (surface_) {
        vkDestroySurfaceKHR(instance_, surface_, nullptr);
        surface_ = VK_NULL_HANDLE;
    }
    if (debugMessenger_) {
        auto destroyMessenger = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
            vkGetInstanceProcAddr(instance_, "vkDestroyDebugUtilsMessengerEXT"));
        if (destroyMessenger) {
            destroyMessenger(instance_, debugMessenger_, nullptr);
        }
        debugMessenger_ = VK_NULL_HANDLE;
    }
    if (instance_) {
        vkDestroyInstance(instance_, nullptr);
        instance_ = VK_NULL_HANDLE;
    }
}

AllocatedBuffer VulkanDevice::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage,
                                           VmaAllocationCreateFlags flags) {
    AllocatedBuffer result{};
    VkBufferCreateInfo bufferInfo{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    bufferInfo.size = size;
    bufferInfo.usage = usage;

    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = memoryUsage;
    allocInfo.flags = flags;

    if (vmaCreateBuffer(allocator_, &bufferInfo, &allocInfo, &result.buffer, &result.allocation, &result.info) !=
        VK_SUCCESS) {
        throw std::runtime_error("vmaCreateBuffer failed");
    }
    return result;
}

void VulkanDevice::destroyBuffer(AllocatedBuffer& buffer) {
    if (buffer.buffer) {
        vmaDestroyBuffer(allocator_, buffer.buffer, buffer.allocation);
        buffer = {};
    }
}

AllocatedImage VulkanDevice::createImage(VkExtent3D extent, VkFormat format, VkImageUsageFlags usage,
                                         VkImageAspectFlags aspect) {
    AllocatedImage result{};
    result.extent = extent;
    result.format = format;

    VkImageCreateInfo imageInfo{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.format = format;
    imageInfo.extent = extent;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.usage = usage;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
    allocInfo.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;

    if (vmaCreateImage(allocator_, &imageInfo, &allocInfo, &result.image, &result.allocation, nullptr) != VK_SUCCESS) {
        throw std::runtime_error("vmaCreateImage failed");
    }

    VkImageViewCreateInfo viewInfo{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
    viewInfo.image = result.image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = format;
    viewInfo.subresourceRange.aspectMask = aspect;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.layerCount = 1;

    if (vkCreateImageView(device_, &viewInfo, nullptr, &result.view) != VK_SUCCESS) {
        throw std::runtime_error("vkCreateImageView failed");
    }
    return result;
}

void VulkanDevice::destroyImage(AllocatedImage& image) {
    if (image.view) {
        vkDestroyImageView(device_, image.view, nullptr);
    }
    if (image.image) {
        vmaDestroyImage(allocator_, image.image, image.allocation);
    }
    image = {};
}

void VulkanDevice::immediateSubmit(void (*fn)(VkCommandBuffer, void*), void* userdata) {
    VkCommandBufferAllocateInfo alloc{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    alloc.commandPool = commandPool_;
    alloc.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    alloc.commandBufferCount = 1;

    VkCommandBuffer cmd = VK_NULL_HANDLE;
    vkAllocateCommandBuffers(device_, &alloc, &cmd);

    VkCommandBufferBeginInfo begin{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cmd, &begin);
    fn(cmd, userdata);
    vkEndCommandBuffer(cmd);

    VkCommandBufferSubmitInfo cmdInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO};
    cmdInfo.commandBuffer = cmd;

    VkSubmitInfo2 submit{VK_STRUCTURE_TYPE_SUBMIT_INFO_2};
    submit.commandBufferInfoCount = 1;
    submit.pCommandBufferInfos = &cmdInfo;

    vkResetFences(device_, 1, &immediateFence_);
    vkQueueSubmit2(graphicsQueue_, 1, &submit, immediateFence_);
    vkWaitForFences(device_, 1, &immediateFence_, VK_TRUE, UINT64_MAX);
    vkFreeCommandBuffers(device_, commandPool_, 1, &cmd);
}

} // namespace nu
