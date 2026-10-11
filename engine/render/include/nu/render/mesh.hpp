#pragma once

#include "nu/math/math.hpp"
#include "nu/render/vulkan_device.hpp"

#include <vector>

namespace nu {

struct Vertex {
    Vec3 position;
    Vec3 normal;
    Vec2 uv;
    Vec4 color{1.0f, 1.0f, 1.0f, 1.0f};
    Vec4 tangent{1.0f, 0.0f, 0.0f, 1.0f}; // xyz + handedness (MikkTSpace)
};

struct GpuMesh {
    AllocatedBuffer vertexBuffer{};
    AllocatedBuffer indexBuffer{};
    uint32_t indexCount = 0;
    uint32_t vertexCount = 0;
};

class MeshFactory {
public:
    explicit MeshFactory(VulkanDevice& device) : device_(&device) {}

    GpuMesh create(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices);
    void destroy(GpuMesh& mesh);

    [[nodiscard]] GpuMesh createTriangle();
    [[nodiscard]] GpuMesh createCube(float size = 1.0f);
    [[nodiscard]] GpuMesh createPlane(float size = 20.0f, int subdivisions = 1);
    [[nodiscard]] GpuMesh createCapsule(float radius = 0.4f, float halfHeight = 0.5f, int segments = 12);

private:
    VulkanDevice* device_ = nullptr;
};

} // namespace nu
