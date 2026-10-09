#include "nu/render/swapchain.hpp"

#include "nu/core/log.hpp"

#include <stdexcept>

namespace nu {

void Swapchain::init(VulkanDevice& device, uint32_t width, uint32_t height) {
    device_ = &device;
    create(width, height);
}

void Swapchain::resize(uint32_t width, uint32_t height) {
    if (!device_ || width == 0 || height == 0) {
        return;
    }
    vkDeviceWaitIdle(device_->device());
    destroy();
    create(width, height);
}

void Swapchain::shutdown() {
    destroy();
    device_ = nullptr;
}

void Swapchain::create(uint32_t width, uint32_t height) {
    vkb::SwapchainBuilder builder{device_->physicalDevice(), device_->device(), device_->surface()};
    auto ret = builder.set_desired_format({VK_FORMAT_B8G8R8A8_SRGB, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR})
                   .set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
                   .set_desired_extent(width, height)
                   .add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)
                   .build();
    if (!ret) {
        throw std::runtime_error("Failed to create swapchain: " + ret.error().message());
    }

    vkb::Swapchain vkbSwapchain = ret.value();
    swapchain_ = vkbSwapchain.swapchain;
    format_ = vkbSwapchain.image_format;
    extent_ = vkbSwapchain.extent;
    images_ = vkbSwapchain.get_images().value();
    views_ = vkbSwapchain.get_image_views().value();

    NU_LOG_INFO("Swapchain {}x{} format={} images={}", extent_.width, extent_.height, static_cast<int>(format_),
                images_.size());
}

void Swapchain::destroy() {
    if (!device_ || !swapchain_) {
        return;
    }
    for (VkImageView view : views_) {
        vkDestroyImageView(device_->device(), view, nullptr);
    }
    views_.clear();
    images_.clear();
    vkDestroySwapchainKHR(device_->device(), swapchain_, nullptr);
    swapchain_ = VK_NULL_HANDLE;
}

std::optional<uint32_t> Swapchain::acquireNext(VkSemaphore signal, uint64_t timeout) {
    uint32_t index = 0;
    const VkResult result =
        vkAcquireNextImageKHR(device_->device(), swapchain_, timeout, signal, VK_NULL_HANDLE, &index);
    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        return std::nullopt;
    }
    if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("vkAcquireNextImageKHR failed");
    }
    return index;
}

VkResult Swapchain::present(VkQueue queue, uint32_t imageIndex, VkSemaphore wait) {
    VkPresentInfoKHR present{VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
    present.waitSemaphoreCount = 1;
    present.pWaitSemaphores = &wait;
    present.swapchainCount = 1;
    present.pSwapchains = &swapchain_;
    present.pImageIndices = &imageIndex;
    return vkQueuePresentKHR(queue, &present);
}

} // namespace nu
