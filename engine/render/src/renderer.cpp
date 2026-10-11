#include "nu/render/renderer.hpp"

#include "nu/assets/texture_loader.hpp"
#include "nu/core/log.hpp"
#include "nu/core/paths.hpp"
#include "nu/platform/filesystem.hpp"

#include <array>
#include <cstring>
#include <stdexcept>
#include <vector>

namespace nu {
namespace {

void transitionImage(VkCommandBuffer cmd, VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout,
                     VkImageAspectFlags aspect, VkPipelineStageFlags2 srcStage, VkAccessFlags2 srcAccess,
                     VkPipelineStageFlags2 dstStage, VkAccessFlags2 dstAccess) {
    VkImageMemoryBarrier2 barrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2};
    barrier.srcStageMask = srcStage;
    barrier.srcAccessMask = srcAccess;
    barrier.dstStageMask = dstStage;
    barrier.dstAccessMask = dstAccess;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = aspect;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.layerCount = 1;

    VkDependencyInfo dep{VK_STRUCTURE_TYPE_DEPENDENCY_INFO};
    dep.imageMemoryBarrierCount = 1;
    dep.pImageMemoryBarriers = &barrier;
    vkCmdPipelineBarrier2(cmd, &dep);
}

} // namespace

Renderer::~Renderer() {
    shutdown();
}

void Renderer::init(SDL_Window* window, uint32_t width, uint32_t height) {
    extent_ = {width, height};
    device_.init(window);
    swapchain_.init(device_, width, height);
    shaders_.init(&device_);
    meshFactory_ = std::make_unique<MeshFactory>(device_);

    createDepth();
    createFrameResources();
    createMaterialResources();
    if (!ensureShaders()) {
        throw std::runtime_error("Failed to load mesh shaders (expected shaders/mesh.vert.spv and mesh.frag.spv)");
    }
    createPipelines();
    ready_ = true;
    NU_LOG_INFO("Renderer initialized");
}

void Renderer::shutdown() {
    if (!ready_ && !device_.device()) {
        return;
    }
    if (device_.device()) {
        vkDeviceWaitIdle(device_.device());
    }
    destroyPipelines();
    destroyMaterialResources();
    destroyFrameResources();
    destroyDepth();
    shaders_.shutdown();
    meshFactory_.reset();
    swapchain_.shutdown();
    device_.shutdown();
    ready_ = false;
}

void Renderer::resize(uint32_t width, uint32_t height) {
    if (width == 0 || height == 0) {
        return;
    }
    framebufferResized_ = true;
    extent_ = {width, height};
}

void Renderer::createDepth() {
    destroyDepth();
    depthImage_ = device_.createImage({extent_.width, extent_.height, 1}, VK_FORMAT_D32_SFLOAT,
                                      VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_IMAGE_ASPECT_DEPTH_BIT);
}

void Renderer::destroyDepth() {
    device_.destroyImage(depthImage_);
}

void Renderer::createFrameResources() {
    VkCommandBufferAllocateInfo alloc{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    alloc.commandPool = device_.commandPool();
    alloc.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    alloc.commandBufferCount = kFramesInFlight;

    std::array<VkCommandBuffer, kFramesInFlight> cmds{};
    vkAllocateCommandBuffers(device_.device(), &alloc, cmds.data());

    VkDescriptorSetLayoutBinding uboBinding{};
    uboBinding.binding = 0;
    uboBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboBinding.descriptorCount = 1;
    uboBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo layoutInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings = &uboBinding;
    vkCreateDescriptorSetLayout(device_.device(), &layoutInfo, nullptr, &frameSetLayout_);

    VkDescriptorPoolSize poolSize{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, kFramesInFlight};
    VkDescriptorPoolCreateInfo poolInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
    poolInfo.maxSets = kFramesInFlight;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;
    vkCreateDescriptorPool(device_.device(), &poolInfo, nullptr, &descriptorPool_);

    for (int i = 0; i < kFramesInFlight; ++i) {
        frames_[i].cmd = cmds[static_cast<size_t>(i)];

        VkSemaphoreCreateInfo semInfo{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
        vkCreateSemaphore(device_.device(), &semInfo, nullptr, &frames_[i].imageAvailable);
        vkCreateSemaphore(device_.device(), &semInfo, nullptr, &frames_[i].renderFinished);

        VkFenceCreateInfo fenceInfo{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        vkCreateFence(device_.device(), &fenceInfo, nullptr, &frames_[i].inFlight);

        frames_[i].frameUbo =
            device_.createBuffer(sizeof(FrameConstants), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_AUTO,
                                 VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
                                     VMA_ALLOCATION_CREATE_MAPPED_BIT);
    }

    std::array<VkDescriptorSetLayout, kFramesInFlight> layouts{};
    layouts.fill(frameSetLayout_);
    VkDescriptorSetAllocateInfo setAlloc{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
    setAlloc.descriptorPool = descriptorPool_;
    setAlloc.descriptorSetCount = kFramesInFlight;
    setAlloc.pSetLayouts = layouts.data();
    vkAllocateDescriptorSets(device_.device(), &setAlloc, frameSets_.data());

    for (int i = 0; i < kFramesInFlight; ++i) {
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = frames_[i].frameUbo.buffer;
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(FrameConstants);

        VkWriteDescriptorSet write{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
        write.dstSet = frameSets_[static_cast<size_t>(i)];
        write.dstBinding = 0;
        write.descriptorCount = 1;
        write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        write.pBufferInfo = &bufferInfo;
        vkUpdateDescriptorSets(device_.device(), 1, &write, 0, nullptr);
    }
}

void Renderer::destroyFrameResources() {
    for (auto& frame : frames_) {
        if (frame.imageAvailable) {
            vkDestroySemaphore(device_.device(), frame.imageAvailable, nullptr);
        }
        if (frame.renderFinished) {
            vkDestroySemaphore(device_.device(), frame.renderFinished, nullptr);
        }
        if (frame.inFlight) {
            vkDestroyFence(device_.device(), frame.inFlight, nullptr);
        }
        device_.destroyBuffer(frame.frameUbo);
        frame = {};
    }
    if (descriptorPool_) {
        vkDestroyDescriptorPool(device_.device(), descriptorPool_, nullptr);
        descriptorPool_ = VK_NULL_HANDLE;
    }
    if (frameSetLayout_) {
        vkDestroyDescriptorSetLayout(device_.device(), frameSetLayout_, nullptr);
        frameSetLayout_ = VK_NULL_HANDLE;
    }
}

namespace {

AllocatedImage makeSolidTexture(VulkanDevice& device, uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    CpuTexture cpu;
    cpu.width = 1;
    cpu.height = 1;
    cpu.channels = 4;
    cpu.pixels = {r, g, b, a};
    return uploadTexture(device, cpu);
}

} // namespace

void Renderer::createMaterialResources() {
    VkSamplerCreateInfo samplerInfo{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.maxLod = 16.0f;
    vkCreateSampler(device_.device(), &samplerInfo, nullptr, &materialSampler_);

    defaultWhite_ = makeSolidTexture(device_, 255, 255, 255, 255);
    defaultNormal_ = makeSolidTexture(device_, 128, 128, 255, 255);
    defaultOrm_ = makeSolidTexture(device_, 255, 128, 0, 255); // AO=1, rough=0.5, metal=0
    defaultEmissive_ = makeSolidTexture(device_, 0, 0, 0, 255);

    std::array<VkDescriptorSetLayoutBinding, 4> bindings{};
    for (uint32_t i = 0; i < 4; ++i) {
        bindings[i].binding = i;
        bindings[i].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        bindings[i].descriptorCount = 1;
        bindings[i].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    }
    VkDescriptorSetLayoutCreateInfo layoutInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();
    vkCreateDescriptorSetLayout(device_.device(), &layoutInfo, nullptr, &materialSetLayout_);

    VkDescriptorPoolSize poolSize{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4 * 128};
    VkDescriptorPoolCreateInfo poolInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
    poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    poolInfo.maxSets = 128;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;
    vkCreateDescriptorPool(device_.device(), &poolInfo, nullptr, &materialDescriptorPool_);

    defaultMaterialSet_ =
        createMaterialSet(defaultWhite_.view, defaultNormal_.view, defaultOrm_.view, defaultEmissive_.view);
}

void Renderer::destroyMaterialResources() {
    defaultMaterialSet_ = VK_NULL_HANDLE;
    if (materialDescriptorPool_) {
        vkDestroyDescriptorPool(device_.device(), materialDescriptorPool_, nullptr);
        materialDescriptorPool_ = VK_NULL_HANDLE;
    }
    if (materialSetLayout_) {
        vkDestroyDescriptorSetLayout(device_.device(), materialSetLayout_, nullptr);
        materialSetLayout_ = VK_NULL_HANDLE;
    }
    if (materialSampler_) {
        vkDestroySampler(device_.device(), materialSampler_, nullptr);
        materialSampler_ = VK_NULL_HANDLE;
    }
    device_.destroyImage(defaultWhite_);
    device_.destroyImage(defaultNormal_);
    device_.destroyImage(defaultOrm_);
    device_.destroyImage(defaultEmissive_);
}

VkDescriptorSet Renderer::createMaterialSet(VkImageView baseColor, VkImageView normal, VkImageView orm,
                                            VkImageView emissive) {
    VkDescriptorSetAllocateInfo alloc{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
    alloc.descriptorPool = materialDescriptorPool_;
    alloc.descriptorSetCount = 1;
    alloc.pSetLayouts = &materialSetLayout_;
    VkDescriptorSet set = VK_NULL_HANDLE;
    if (vkAllocateDescriptorSets(device_.device(), &alloc, &set) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate material descriptor set");
    }

    const VkImageView views[4] = {
        baseColor ? baseColor : defaultWhite_.view,
        normal ? normal : defaultNormal_.view,
        orm ? orm : defaultOrm_.view,
        emissive ? emissive : defaultEmissive_.view,
    };

    std::array<VkDescriptorImageInfo, 4> infos{};
    std::array<VkWriteDescriptorSet, 4> writes{};
    for (uint32_t i = 0; i < 4; ++i) {
        infos[i].sampler = materialSampler_;
        infos[i].imageView = views[i];
        infos[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        writes[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[i].dstSet = set;
        writes[i].dstBinding = i;
        writes[i].descriptorCount = 1;
        writes[i].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writes[i].pImageInfo = &infos[i];
    }
    vkUpdateDescriptorSets(device_.device(), static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
    return set;
}

bool Renderer::ensureShaders() {
    const auto vertPath = Paths::resolveShader("mesh.vert.spv");
    const auto fragPath = Paths::resolveShader("mesh.frag.spv");

    // Try compile from GLSL if SPIR-V missing.
    if (!std::filesystem::exists(vertPath)) {
        const auto glsl = Paths::resolveShader("mesh.vert");
        auto spirv = compileGlslToSpirv(glsl, "vert");
        if (!spirv.empty()) {
            writeBinaryFile(vertPath, spirv.data(), spirv.size() * sizeof(uint32_t));
        }
    }
    if (!std::filesystem::exists(fragPath)) {
        const auto glsl = Paths::resolveShader("mesh.frag");
        auto spirv = compileGlslToSpirv(glsl, "frag");
        if (!spirv.empty()) {
            writeBinaryFile(fragPath, spirv.data(), spirv.size() * sizeof(uint32_t));
        }
    }

    vertModule_ = shaders_.loadSpirvFile(vertPath);
    fragModule_ = shaders_.loadSpirvFile(fragPath);
    return vertModule_ && fragModule_;
}

void Renderer::createPipelines() {
    VkPushConstantRange push{};
    push.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    push.offset = 0;
    push.size = sizeof(Mat4) + sizeof(MaterialGpu);

    std::array<VkDescriptorSetLayout, 2> setLayouts = {frameSetLayout_, materialSetLayout_};
    VkPipelineLayoutCreateInfo layoutInfo{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    layoutInfo.setLayoutCount = static_cast<uint32_t>(setLayouts.size());
    layoutInfo.pSetLayouts = setLayouts.data();
    layoutInfo.pushConstantRangeCount = 1;
    layoutInfo.pPushConstantRanges = &push;
    vkCreatePipelineLayout(device_.device(), &layoutInfo, nullptr, &pipelineLayout_);

    VkPipelineShaderStageCreateInfo stages[2]{};
    stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    stages[0].module = vertModule_;
    stages[0].pName = "main";
    stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    stages[1].module = fragModule_;
    stages[1].pName = "main";

    VkVertexInputBindingDescription binding{};
    binding.binding = 0;
    binding.stride = sizeof(Vertex);
    binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    std::array<VkVertexInputAttributeDescription, 5> attrs{};
    attrs[0] = {0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, position)};
    attrs[1] = {1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, normal)};
    attrs[2] = {2, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, uv)};
    attrs[3] = {3, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(Vertex, color)};
    attrs[4] = {4, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(Vertex, tangent)};

    VkPipelineVertexInputStateCreateInfo vertexInput{VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
    vertexInput.vertexBindingDescriptionCount = 1;
    vertexInput.pVertexBindingDescriptions = &binding;
    vertexInput.vertexAttributeDescriptionCount = static_cast<uint32_t>(attrs.size());
    vertexInput.pVertexAttributeDescriptions = attrs.data();

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    VkPipelineViewportStateCreateInfo viewportState{VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo raster{VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
    raster.polygonMode = VK_POLYGON_MODE_FILL;
    raster.cullMode = VK_CULL_MODE_BACK_BIT;
    raster.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    raster.lineWidth = 1.0f;

    VkPipelineMultisampleStateCreateInfo msaa{VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
    msaa.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineDepthStencilStateCreateInfo depth{VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
    depth.depthTestEnable = VK_TRUE;
    depth.depthWriteEnable = VK_TRUE;
    depth.depthCompareOp = VK_COMPARE_OP_LESS;

    VkPipelineColorBlendAttachmentState blendAttachment{};
    blendAttachment.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

    VkPipelineColorBlendStateCreateInfo blend{VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
    blend.attachmentCount = 1;
    blend.pAttachments = &blendAttachment;

    std::array<VkDynamicState, 2> dynamicStates = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dynamic{VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};
    dynamic.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamic.pDynamicStates = dynamicStates.data();

    VkPipelineRenderingCreateInfo rendering{VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO};
    VkFormat colorFormat = swapchain_.format();
    rendering.colorAttachmentCount = 1;
    rendering.pColorAttachmentFormats = &colorFormat;
    rendering.depthAttachmentFormat = depthImage_.format;

    VkGraphicsPipelineCreateInfo pipelineInfo{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
    pipelineInfo.pNext = &rendering;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = stages;
    pipelineInfo.pVertexInputState = &vertexInput;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &raster;
    pipelineInfo.pMultisampleState = &msaa;
    pipelineInfo.pDepthStencilState = &depth;
    pipelineInfo.pColorBlendState = &blend;
    pipelineInfo.pDynamicState = &dynamic;
    pipelineInfo.layout = pipelineLayout_;

    if (vkCreateGraphicsPipelines(device_.device(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline_) !=
        VK_SUCCESS) {
        throw std::runtime_error("Failed to create graphics pipeline");
    }
}

void Renderer::destroyPipelines() {
    if (pipeline_) {
        vkDestroyPipeline(device_.device(), pipeline_, nullptr);
        pipeline_ = VK_NULL_HANDLE;
    }
    if (pipelineLayout_) {
        vkDestroyPipelineLayout(device_.device(), pipelineLayout_, nullptr);
        pipelineLayout_ = VK_NULL_HANDLE;
    }
}

void Renderer::beginFrame() {
    if (shaders_.pollHotReload()) {
        vkDeviceWaitIdle(device_.device());
        destroyPipelines();
        vertModule_ = shaders_.loadSpirvFile(Paths::resolveShader("mesh.vert.spv"));
        fragModule_ = shaders_.loadSpirvFile(Paths::resolveShader("mesh.frag.spv"));
        createPipelines();
    }

    if (framebufferResized_) {
        vkDeviceWaitIdle(device_.device());
        swapchain_.resize(extent_.width, extent_.height);
        createDepth();
        framebufferResized_ = false;
    }

    FrameData& frame = frames_[frameIndex_];
    vkWaitForFences(device_.device(), 1, &frame.inFlight, VK_TRUE, UINT64_MAX);

    auto image = swapchain_.acquireNext(frame.imageAvailable);
    if (!image) {
        framebufferResized_ = true;
        frameActive_ = false;
        return;
    }
    imageIndex_ = *image;
    vkResetFences(device_.device(), 1, &frame.inFlight);
    vkResetCommandBuffer(frame.cmd, 0);

    VkCommandBufferBeginInfo begin{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(frame.cmd, &begin);
    frameActive_ = true;
}

void Renderer::drawScene(const Camera& camera, const RenderScene& scene) {
    FrameData& frame = frames_[frameIndex_];
    if (!frameActive_ || !frame.cmd) {
        return;
    }

    FrameConstants constants{};
    constants.viewProj = camera.viewProjection();
    constants.view = camera.view();
    constants.cameraPos = Vec4{camera.position(), 1.0f};
    constants.lightDir = Vec4{glm::normalize(scene.lightDir), 0.0f};
    constants.lightColor = Vec4{scene.lightColor, 1.0f};
    constants.fogColor = Vec4{scene.fogColor, scene.fogDensity};
    constants.ambientColor = Vec4{scene.ambientColor, 1.0f};
    std::memcpy(frame.frameUbo.info.pMappedData, &constants, sizeof(constants));

    VkImage swapImage = swapchain_.images()[imageIndex_];
    VkImageView swapView = swapchain_.views()[imageIndex_];

    transitionImage(frame.cmd, swapImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                    VK_IMAGE_ASPECT_COLOR_BIT, VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT, 0,
                    VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT);

    transitionImage(frame.cmd, depthImage_.image, VK_IMAGE_LAYOUT_UNDEFINED,
                    VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL, VK_IMAGE_ASPECT_DEPTH_BIT,
                    VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT, 0, VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT,
                    VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT);

    VkRenderingAttachmentInfo colorAttachment{VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO};
    colorAttachment.imageView = swapView;
    colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.clearValue.color = {
        {scene.clearColor.r, scene.clearColor.g, scene.clearColor.b, scene.clearColor.a}};

    VkRenderingAttachmentInfo depthAttachment{VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO};
    depthAttachment.imageView = depthImage_.view;
    depthAttachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.clearValue.depthStencil = {1.0f, 0};

    VkRenderingInfo rendering{VK_STRUCTURE_TYPE_RENDERING_INFO};
    rendering.renderArea.extent = swapchain_.extent();
    rendering.layerCount = 1;
    rendering.colorAttachmentCount = 1;
    rendering.pColorAttachments = &colorAttachment;
    rendering.pDepthAttachment = &depthAttachment;

    vkCmdBeginRendering(frame.cmd, &rendering);

    VkViewport viewport{};
    viewport.width = static_cast<float>(swapchain_.extent().width);
    viewport.height = static_cast<float>(swapchain_.extent().height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(frame.cmd, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.extent = swapchain_.extent();
    vkCmdSetScissor(frame.cmd, 0, 1, &scissor);

    vkCmdBindPipeline(frame.cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_);
    vkCmdBindDescriptorSets(frame.cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout_, 0, 1,
                            &frameSets_[frameIndex_], 0, nullptr);

    struct PushData {
        Mat4 model;
        MaterialGpu material;
    };

    VkDescriptorSet lastMaterial = VK_NULL_HANDLE;
    for (const DrawItem& item : scene.draws) {
        if (!item.mesh || item.mesh->indexCount == 0) {
            continue;
        }
        VkDescriptorSet matSet = item.materialSet ? item.materialSet : defaultMaterialSet_;
        if (matSet != lastMaterial) {
            vkCmdBindDescriptorSets(frame.cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout_, 1, 1, &matSet, 0,
                                    nullptr);
            lastMaterial = matSet;
        }

        PushData push{item.transform, item.material};
        vkCmdPushConstants(frame.cmd, pipelineLayout_, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0,
                           sizeof(PushData), &push);

        VkDeviceSize offset = 0;
        vkCmdBindVertexBuffers(frame.cmd, 0, 1, &item.mesh->vertexBuffer.buffer, &offset);
        vkCmdBindIndexBuffer(frame.cmd, item.mesh->indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
        vkCmdDrawIndexed(frame.cmd, item.mesh->indexCount, 1, 0, 0, 0);
    }

    vkCmdEndRendering(frame.cmd);
}

void Renderer::endFrame(const std::function<void(VkCommandBuffer)>& imguiDraw) {
    FrameData& frame = frames_[frameIndex_];
    if (!frameActive_ || !frame.cmd) {
        return;
    }

    if (imguiDraw) {
        // ImGui draws into the swapchain image in COLOR_ATTACHMENT layout after main pass.
        VkImage swapImage = swapchain_.images()[imageIndex_];
        VkImageView swapView = swapchain_.views()[imageIndex_];

        VkRenderingAttachmentInfo colorAttachment{VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO};
        colorAttachment.imageView = swapView;
        colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

        VkRenderingInfo rendering{VK_STRUCTURE_TYPE_RENDERING_INFO};
        rendering.renderArea.extent = swapchain_.extent();
        rendering.layerCount = 1;
        rendering.colorAttachmentCount = 1;
        rendering.pColorAttachments = &colorAttachment;
        vkCmdBeginRendering(frame.cmd, &rendering);
        imguiDraw(frame.cmd);
        vkCmdEndRendering(frame.cmd);

        (void)swapImage;
    }

    transitionImage(frame.cmd, swapchain_.images()[imageIndex_], VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                    VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_IMAGE_ASPECT_COLOR_BIT,
                    VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
                    VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT, 0);

    vkEndCommandBuffer(frame.cmd);

    VkCommandBufferSubmitInfo cmdInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO};
    cmdInfo.commandBuffer = frame.cmd;

    VkSemaphoreSubmitInfo waitInfo{VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO};
    waitInfo.semaphore = frame.imageAvailable;
    waitInfo.stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;

    VkSemaphoreSubmitInfo signalInfo{VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO};
    signalInfo.semaphore = frame.renderFinished;
    signalInfo.stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;

    VkSubmitInfo2 submit{VK_STRUCTURE_TYPE_SUBMIT_INFO_2};
    submit.waitSemaphoreInfoCount = 1;
    submit.pWaitSemaphoreInfos = &waitInfo;
    submit.commandBufferInfoCount = 1;
    submit.pCommandBufferInfos = &cmdInfo;
    submit.signalSemaphoreInfoCount = 1;
    submit.pSignalSemaphoreInfos = &signalInfo;

    vkQueueSubmit2(device_.graphicsQueue(), 1, &submit, frame.inFlight);

    const VkResult presentResult =
        swapchain_.present(device_.graphicsQueue(), imageIndex_, frame.renderFinished);
    if (presentResult == VK_ERROR_OUT_OF_DATE_KHR || presentResult == VK_SUBOPTIMAL_KHR) {
        framebufferResized_ = true;
    }

    frameActive_ = false;
    frameIndex_ = (frameIndex_ + 1) % kFramesInFlight;
}

} // namespace nu
