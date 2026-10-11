#pragma once

#include "nu/render/camera.hpp"
#include "nu/render/material.hpp"
#include "nu/render/mesh.hpp"
#include "nu/render/shader.hpp"
#include "nu/render/swapchain.hpp"
#include "nu/render/vulkan_device.hpp"

#include <array>
#include <functional>
#include <memory>
#include <vector>

struct SDL_Window;

namespace nu {

struct DrawItem {
    const GpuMesh* mesh = nullptr;
    Mat4 transform{1.0f};
    MaterialGpu material{};
    /// Optional material texture set (set=1). Null uses the renderer default (white/flat).
    VkDescriptorSet materialSet = VK_NULL_HANDLE;
};

struct RenderScene {
    std::vector<DrawItem> draws;
    Vec3 lightDir{-0.35f, -0.85f, -0.25f};
    Vec3 lightColor{1.4f, 1.15f, 0.85f}; // dawn
    Vec3 ambientColor{0.12f, 0.13f, 0.18f};
    Vec3 fogColor{0.35f, 0.38f, 0.42f};
    float fogDensity = 0.035f;
    Vec4 clearColor{0.22f, 0.24f, 0.28f, 1.0f}; // stagnant dawn
};

class Renderer {
public:
    static constexpr int kFramesInFlight = 2;

    Renderer() = default;
    ~Renderer();

    void init(SDL_Window* window, uint32_t width, uint32_t height);
    void shutdown();
    void resize(uint32_t width, uint32_t height);
    /// Block until the GPU finishes all submitted work (safe to destroy GPU resources).
    void waitIdle();

    void beginFrame();
    void drawScene(const Camera& camera, const RenderScene& scene);
    void endFrame(const std::function<void(VkCommandBuffer)>& imguiDraw = {});

    [[nodiscard]] VulkanDevice& device() { return device_; }
    [[nodiscard]] MeshFactory& meshes() { return *meshFactory_; }
    [[nodiscard]] ShaderCache& shaders() { return shaders_; }
    [[nodiscard]] VkDescriptorSetLayout materialSetLayout() const { return materialSetLayout_; }
    [[nodiscard]] VkDescriptorPool materialDescriptorPool() const { return materialDescriptorPool_; }
    [[nodiscard]] VkSampler materialSampler() const { return materialSampler_; }
    [[nodiscard]] VkDescriptorSet defaultMaterialSet() const { return defaultMaterialSet_; }
    [[nodiscard]] VkCommandBuffer currentCmd() const { return frames_[frameIndex_].cmd; }

    /// Allocate a set=1 descriptor for baseColor/normal/ORM/emissive image views.
    [[nodiscard]] VkDescriptorSet createMaterialSet(VkImageView baseColor, VkImageView normal, VkImageView orm,
                                                    VkImageView emissive);
    [[nodiscard]] float aspect() const {
        return extent_.height > 0 ? static_cast<float>(extent_.width) / static_cast<float>(extent_.height) : 1.0f;
    }
    [[nodiscard]] bool ready() const { return ready_; }

    /// For ImGui Vulkan init.
    [[nodiscard]] VkRenderPass imguiCompatFormat() const { return VK_NULL_HANDLE; } // dynamic rendering
    [[nodiscard]] VkFormat swapchainFormat() const { return swapchain_.format(); }
    [[nodiscard]] VkFormat depthFormat() const { return depthImage_.format; }
    [[nodiscard]] uint32_t imageCount() const { return static_cast<uint32_t>(swapchain_.images().size()); }

private:
    struct FrameData {
        VkCommandBuffer cmd = VK_NULL_HANDLE;
        VkSemaphore imageAvailable = VK_NULL_HANDLE;
        VkFence inFlight = VK_NULL_HANDLE;
        AllocatedBuffer frameUbo{};
    };

    void createPipelines();
    void destroyPipelines();
    void createDepth();
    void destroyDepth();
    void createFrameResources();
    void destroyFrameResources();
    void createSwapchainSync();
    void destroySwapchainSync();
    void createMaterialResources();
    void destroyMaterialResources();
    bool ensureShaders();

    VulkanDevice device_;
    Swapchain swapchain_;
    ShaderCache shaders_;
    std::unique_ptr<MeshFactory> meshFactory_;

    VkPipelineLayout pipelineLayout_ = VK_NULL_HANDLE;
    VkPipeline pipeline_ = VK_NULL_HANDLE;
    VkDescriptorSetLayout frameSetLayout_ = VK_NULL_HANDLE;
    VkDescriptorSetLayout materialSetLayout_ = VK_NULL_HANDLE;
    VkDescriptorPool descriptorPool_ = VK_NULL_HANDLE;
    VkDescriptorPool materialDescriptorPool_ = VK_NULL_HANDLE;
    VkSampler materialSampler_ = VK_NULL_HANDLE;
    VkDescriptorSet defaultMaterialSet_ = VK_NULL_HANDLE;
    AllocatedImage defaultWhite_{};
    AllocatedImage defaultNormal_{};
    AllocatedImage defaultOrm_{};
    AllocatedImage defaultEmissive_{};
    std::array<VkDescriptorSet, kFramesInFlight> frameSets_{};

    AllocatedImage depthImage_{};
    VkExtent2D extent_{};
    std::array<FrameData, kFramesInFlight> frames_{};
    /// One render-finished semaphore per swapchain image (present may still hold it).
    std::vector<VkSemaphore> renderFinished_;
    /// Fence currently using each swapchain image (or VK_NULL_HANDLE).
    std::vector<VkFence> imagesInFlight_;
    uint32_t frameIndex_ = 0;
    uint32_t imageIndex_ = 0;
    bool ready_ = false;
    bool framebufferResized_ = false;
    bool frameActive_ = false;

    VkShaderModule vertModule_ = VK_NULL_HANDLE;
    VkShaderModule fragModule_ = VK_NULL_HANDLE;
};

} // namespace nu
