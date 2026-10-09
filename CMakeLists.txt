cmake_minimum_required(VERSION 3.28)

project(NineUnder
    VERSION 0.1.0
    DESCRIPTION "Nine Under — 3D action-roguelite"
    LANGUAGES CXX C
)

# ---------------------------------------------------------------------------
# Options
# ---------------------------------------------------------------------------
option(NU_ENABLE_VALIDATION "Enable Vulkan validation layers" ON)
option(NU_ENABLE_ASSERTS "Enable runtime asserts" ON)
option(NU_ENABLE_ASAN "Enable AddressSanitizer (MSVC / Debug)" OFF)
option(NU_ENABLE_TRACY "Enable Tracy profiler" ON)
option(NU_BUILD_TESTS "Build unit tests" ON)
option(NU_BUILD_TOOLS "Build asset / shader tools" ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ---------------------------------------------------------------------------
# Global compile flags (MSVC)
# ---------------------------------------------------------------------------
if(MSVC)
    add_compile_options(/W4 /permissive- /Zc:preprocessor /MP)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS NOMINMAX WIN32_LEAN_AND_MEAN)

    if(NU_ENABLE_ASAN)
        add_compile_options(/fsanitize=address)
        add_link_options(/INCREMENTAL:NO)
    endif()
endif()

# ---------------------------------------------------------------------------
# Feature macros
# ---------------------------------------------------------------------------
add_compile_definitions(
    $<$<BOOL:${NU_ENABLE_VALIDATION}>:NU_VALIDATION=1>
    $<$<BOOL:${NU_ENABLE_ASSERTS}>:NU_ASSERTS=1>
    $<$<BOOL:${NU_ENABLE_TRACY}>:TRACY_ENABLE>
    NU_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
    NU_VERSION_MINOR=${PROJECT_VERSION_MINOR}
    NU_VERSION_PATCH=${PROJECT_VERSION_PATCH}
)

# ---------------------------------------------------------------------------
# Dependencies (vcpkg)
# ---------------------------------------------------------------------------
find_package(SDL3 CONFIG REQUIRED)
find_package(volk CONFIG REQUIRED)
find_package(VulkanMemoryAllocator CONFIG REQUIRED)
find_package(vk-bootstrap CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(EnTT CONFIG REQUIRED)
find_package(fastgltf CONFIG REQUIRED)
find_package(Ktx CONFIG REQUIRED)
find_package(meshoptimizer CONFIG REQUIRED)
find_package(Jolt CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)

# miniaudio is header-only in current vcpkg (no CONFIG package)
find_path(MINIAUDIO_INCLUDE_DIRS "miniaudio.h" REQUIRED)

# stb (vcpkg port names vary)
find_path(STB_INCLUDE_DIRS "stb_image.h" HINTS ${Stb_INCLUDE_DIR})
if(NOT STB_INCLUDE_DIRS)
    find_package(Stb REQUIRED)
    set(STB_INCLUDE_DIRS ${Stb_INCLUDE_DIR})
endif()

if(NU_ENABLE_TRACY)
    find_package(Tracy CONFIG REQUIRED)
endif()

if(NU_BUILD_TESTS)
    find_package(doctest CONFIG REQUIRED)
    enable_testing()
endif()

# Vulkan headers / loader (SDK or vcpkg)
find_package(Vulkan REQUIRED)

# ---------------------------------------------------------------------------
# Data / shader paths (compile-time defaults for runtime relative lookup)
# ---------------------------------------------------------------------------
set(NU_SOURCE_DIR "${CMAKE_SOURCE_DIR}")
set(NU_SHADER_SOURCE_DIR "${CMAKE_SOURCE_DIR}/shaders")
set(NU_ASSET_SOURCE_DIR "${CMAKE_SOURCE_DIR}/assets")

# ---------------------------------------------------------------------------
# Copy / stage runtime data next to the game binary
# ---------------------------------------------------------------------------
function(nu_stage_runtime_data target_name)
    add_custom_command(TARGET ${target_name} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:${target_name}>/data
        COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:${target_name}>/shaders
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${NU_ASSET_SOURCE_DIR}
            $<TARGET_FILE_DIR:${target_name}>/data
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${NU_SHADER_SOURCE_DIR}
            $<TARGET_FILE_DIR:${target_name}>/shaders
        # Use the system Vulkan loader; vcpkg's applocal copy can break ICD discovery.
        COMMAND ${CMAKE_COMMAND} -E rm -f $<TARGET_FILE_DIR:${target_name}>/vulkan-1.dll
        COMMENT "Staging assets and shaders for ${target_name}"
    )
endfunction()

# ---------------------------------------------------------------------------
# Subdirs
# ---------------------------------------------------------------------------
add_subdirectory(engine)
add_subdirectory(game)

if(NU_BUILD_TOOLS)
    add_subdirectory(tools)
endif()

if(NU_BUILD_TESTS)
    add_subdirectory(tests)
endif()

message(STATUS "Nine Under ${PROJECT_VERSION}")
message(STATUS "  Validation : ${NU_ENABLE_VALIDATION}")
message(STATUS "  Asserts    : ${NU_ENABLE_ASSERTS}")
message(STATUS "  Tracy      : ${NU_ENABLE_TRACY}")
message(STATUS "  ASan       : ${NU_ENABLE_ASAN}")
