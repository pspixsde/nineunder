add_executable(nu_asset_cooker
    src/asset_cooker.cpp
)

target_link_libraries(nu_asset_cooker PRIVATE nu::engine)

# Optional: compile GLSL shaders to SPIR-V when glslangValidator is available.
find_program(GLSLANG_VALIDATOR NAMES glslangValidator glslang HINTS ENV VULKAN_SDK PATH_SUFFIXES Bin bin)
if(GLSLANG_VALIDATOR)
    set(NU_SHADER_SOURCES
        ${CMAKE_SOURCE_DIR}/shaders/mesh.vert
        ${CMAKE_SOURCE_DIR}/shaders/mesh.frag
    )
    set(NU_SHADER_OUTPUTS
        ${CMAKE_SOURCE_DIR}/shaders/mesh.vert.spv
        ${CMAKE_SOURCE_DIR}/shaders/mesh.frag.spv
    )

    add_custom_command(
        OUTPUT ${CMAKE_SOURCE_DIR}/shaders/mesh.vert.spv
        COMMAND ${GLSLANG_VALIDATOR} -V -S vert ${CMAKE_SOURCE_DIR}/shaders/mesh.vert -o ${CMAKE_SOURCE_DIR}/shaders/mesh.vert.spv
        DEPENDS ${CMAKE_SOURCE_DIR}/shaders/mesh.vert
        COMMENT "Compiling mesh.vert → SPIR-V"
    )
    add_custom_command(
        OUTPUT ${CMAKE_SOURCE_DIR}/shaders/mesh.frag.spv
        COMMAND ${GLSLANG_VALIDATOR} -V -S frag ${CMAKE_SOURCE_DIR}/shaders/mesh.frag -o ${CMAKE_SOURCE_DIR}/shaders/mesh.frag.spv
        DEPENDS ${CMAKE_SOURCE_DIR}/shaders/mesh.frag
        COMMENT "Compiling mesh.frag → SPIR-V"
    )

    add_custom_target(nu_shaders ALL DEPENDS ${NU_SHADER_OUTPUTS})
    add_dependencies(NineUnder nu_shaders)
    message(STATUS "glslangValidator found — shader compile target enabled")
else()
    message(WARNING "glslangValidator not found — run tools/compile_shaders.ps1 after installing the Vulkan SDK")
endif()
