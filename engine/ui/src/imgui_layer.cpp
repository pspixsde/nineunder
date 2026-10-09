#include "nu/ui/imgui_layer.hpp"

#include "nu/core/log.hpp"

#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_vulkan.h>

#include <algorithm>
#include <stdexcept>

namespace nu {
namespace {

void checkVk(VkResult err) {
    if (err != 0) {
        throw std::runtime_error("ImGui Vulkan error");
    }
}

} // namespace

void ImGuiLayer::init(SDL_Window* window, Renderer& renderer) {
    renderer_ = &renderer;

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
#ifdef ImGuiConfigFlags_DockingEnable
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
#endif
    ImGui::StyleColorsDark();

    VkDescriptorPoolSize poolSizes[] = {
        {VK_DESCRIPTOR_TYPE_SAMPLER, 1000},
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000},
        {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000},
        {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000},
        {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000},
        {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000},
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000},
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000},
        {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000},
    };
    VkDescriptorPoolCreateInfo poolInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
    poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    poolInfo.maxSets = 1000;
    poolInfo.poolSizeCount = static_cast<uint32_t>(std::size(poolSizes));
    poolInfo.pPoolSizes = poolSizes;
    checkVk(vkCreateDescriptorPool(renderer.device().device(), &poolInfo, nullptr, &imguiPool_));

    ImGui_ImplSDL3_InitForVulkan(window);

    ImGui_ImplVulkan_InitInfo initInfo{};
    initInfo.Instance = renderer.device().instance();
    initInfo.PhysicalDevice = renderer.device().physicalDevice();
    initInfo.Device = renderer.device().device();
    initInfo.QueueFamily = renderer.device().graphicsQueueFamily();
    initInfo.Queue = renderer.device().graphicsQueue();
    initInfo.DescriptorPool = imguiPool_;
    initInfo.MinImageCount = 2;
    initInfo.ImageCount = std::max(2u, renderer.imageCount());
    initInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    initInfo.CheckVkResultFn = checkVk;

#if defined(IMGUI_IMPL_VULKAN_HAS_DYNAMIC_RENDERING) || (IMGUI_VERSION_NUM >= 18990)
    initInfo.UseDynamicRendering = true;
    VkFormat colorFormat = renderer.swapchainFormat();
    initInfo.PipelineRenderingCreateInfo = {};
    initInfo.PipelineRenderingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    initInfo.PipelineRenderingCreateInfo.colorAttachmentCount = 1;
    initInfo.PipelineRenderingCreateInfo.pColorAttachmentFormats = &colorFormat;
#endif

    if (!ImGui_ImplVulkan_Init(&initInfo)) {
        throw std::runtime_error("ImGui_ImplVulkan_Init failed");
    }
    ready_ = true;
    NU_LOG_INFO("ImGui layer ready");
}

void ImGuiLayer::shutdown() {
    if (!ready_ || !renderer_) {
        return;
    }
    vkDeviceWaitIdle(renderer_->device().device());
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
    if (imguiPool_) {
        vkDestroyDescriptorPool(renderer_->device().device(), imguiPool_, nullptr);
        imguiPool_ = VK_NULL_HANDLE;
    }
    ready_ = false;
    renderer_ = nullptr;
}

void ImGuiLayer::processEvent(const SDL_Event& event) {
    if (ready_) {
        ImGui_ImplSDL3_ProcessEvent(&event);
    }
}

void ImGuiLayer::beginFrame() {
    if (!ready_) {
        return;
    }
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();
}

void ImGuiLayer::endFrame(VkCommandBuffer cmd) {
    if (!ready_) {
        return;
    }
    ImGui::Render();
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);
}

} // namespace nu
