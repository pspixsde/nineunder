# nu_engine — game-agnostic static library

set(NU_ENGINE_SOURCES
    # core
    core/src/log.cpp
    core/src/assert.cpp
    core/src/time.cpp
    core/src/paths.cpp
    # math
    math/src/math.cpp
    # platform
    platform/src/window.cpp
    platform/src/input.cpp
    platform/src/filesystem.cpp
    # render
    render/src/vulkan_device.cpp
    render/src/swapchain.cpp
    render/src/renderer.cpp
    render/src/shader.cpp
    render/src/mesh.cpp
    render/src/camera.cpp
    render/src/material.cpp
    # assets
    assets/src/asset_registry.cpp
    assets/src/gltf_loader.cpp
    assets/src/texture_loader.cpp
    # scene
    scene/src/world.cpp
    scene/src/transform.cpp
    # physics
    physics/src/physics_world.cpp
    physics/src/character_controller.cpp
    # audio
    audio/src/audio_engine.cpp
    # ui
    ui/src/imgui_layer.cpp
    # app glue
    src/application.cpp
)

# Ensure consumers can include nu/* from all modules
set(NU_ENGINE_PUBLIC_INCLUDES
    ${CMAKE_CURRENT_SOURCE_DIR}/core/include
    ${CMAKE_CURRENT_SOURCE_DIR}/math/include
    ${CMAKE_CURRENT_SOURCE_DIR}/platform/include
    ${CMAKE_CURRENT_SOURCE_DIR}/render/include
    ${CMAKE_CURRENT_SOURCE_DIR}/assets/include
    ${CMAKE_CURRENT_SOURCE_DIR}/scene/include
    ${CMAKE_CURRENT_SOURCE_DIR}/physics/include
    ${CMAKE_CURRENT_SOURCE_DIR}/audio/include
    ${CMAKE_CURRENT_SOURCE_DIR}/ui/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

add_library(nu_engine STATIC ${NU_ENGINE_SOURCES})
add_library(nu::engine ALIAS nu_engine)

target_include_directories(nu_engine
    PUBLIC
        ${NU_ENGINE_PUBLIC_INCLUDES}
        ${STB_INCLUDE_DIRS}
)

# Normalize package imported-target names across vcpkg versions
if(TARGET GPUOpen::VulkanMemoryAllocator)
    set(NU_VMA_TARGET GPUOpen::VulkanMemoryAllocator)
elseif(TARGET unofficial::vulkan-memory-allocator)
    set(NU_VMA_TARGET unofficial::vulkan-memory-allocator)
else()
    set(NU_VMA_TARGET VulkanMemoryAllocator)
endif()

if(TARGET miniaudio::miniaudio)
    set(NU_MINIAUDIO_TARGET miniaudio::miniaudio)
elseif(TARGET unofficial::miniaudio)
    set(NU_MINIAUDIO_TARGET unofficial::miniaudio)
else()
    set(NU_MINIAUDIO_TARGET miniaudio)
endif()

target_link_libraries(nu_engine
    PUBLIC
        SDL3::SDL3
        volk::volk
        ${NU_VMA_TARGET}
        vk-bootstrap::vk-bootstrap
        glm::glm
        EnTT::EnTT
        fastgltf::fastgltf
        KTX::ktx
        meshoptimizer::meshoptimizer
        Jolt::Jolt
        ${NU_MINIAUDIO_TARGET}
        imgui::imgui
        spdlog::spdlog
        Vulkan::Vulkan
)

if(NU_ENABLE_TRACY)
    target_link_libraries(nu_engine PUBLIC Tracy::TracyClient)
endif()

target_compile_definitions(nu_engine
    PUBLIC
        GLM_FORCE_DEPTH_ZERO_TO_ONE
        GLM_FORCE_RADIANS
        VK_NO_PROTOTYPES
        $<$<BOOL:${WIN32}>:VK_USE_PLATFORM_WIN32_KHR>
    PRIVATE
        NU_SHADER_DIR="${NU_SHADER_SOURCE_DIR}"
        NU_ASSET_DIR="${NU_ASSET_SOURCE_DIR}"
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${NU_ENGINE_SOURCES})
