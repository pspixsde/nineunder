#include "nu/render/mesh.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace nu {

GpuMesh MeshFactory::create(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices) {
    GpuMesh mesh{};
    mesh.vertexCount = static_cast<uint32_t>(vertices.size());
    mesh.indexCount = static_cast<uint32_t>(indices.size());

    const VkDeviceSize vSize = sizeof(Vertex) * vertices.size();
    const VkDeviceSize iSize = sizeof(uint32_t) * indices.size();

    mesh.vertexBuffer = device_->createBuffer(vSize,
                                              VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                              VMA_MEMORY_USAGE_AUTO,
                                              VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
                                                  VMA_ALLOCATION_CREATE_MAPPED_BIT);
    std::memcpy(mesh.vertexBuffer.info.pMappedData, vertices.data(), static_cast<size_t>(vSize));

    mesh.indexBuffer = device_->createBuffer(iSize,
                                             VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                             VMA_MEMORY_USAGE_AUTO,
                                             VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
                                                 VMA_ALLOCATION_CREATE_MAPPED_BIT);
    std::memcpy(mesh.indexBuffer.info.pMappedData, indices.data(), static_cast<size_t>(iSize));

    return mesh;
}

void MeshFactory::destroy(GpuMesh& mesh) {
    device_->destroyBuffer(mesh.vertexBuffer);
    device_->destroyBuffer(mesh.indexBuffer);
    mesh = {};
}

GpuMesh MeshFactory::createTriangle() {
    const std::vector<Vertex> verts = {
        {{-0.8f, -0.8f, 0.0f}, {0, 0, 1}, {0, 0}, {1, 0.2f, 0.2f, 1}},
        {{0.8f, -0.8f, 0.0f}, {0, 0, 1}, {1, 0}, {0.2f, 1, 0.2f, 1}},
        {{0.0f, 0.8f, 0.0f}, {0, 0, 1}, {0.5f, 1}, {0.2f, 0.4f, 1, 1}},
    };
    const std::vector<uint32_t> indices = {0, 1, 2};
    return create(verts, indices);
}

GpuMesh MeshFactory::createCube(float size) {
    const float h = size * 0.5f;
    std::vector<Vertex> v;
    std::vector<uint32_t> idx;

    auto addFace = [&](Vec3 n, Vec3 a, Vec3 b, Vec3 c, Vec3 d, Vec4 color) {
        const uint32_t base = static_cast<uint32_t>(v.size());
        v.push_back({a, n, {0, 0}, color});
        v.push_back({b, n, {1, 0}, color});
        v.push_back({c, n, {1, 1}, color});
        v.push_back({d, n, {0, 1}, color});
        idx.push_back(base + 0);
        idx.push_back(base + 1);
        idx.push_back(base + 2);
        idx.push_back(base + 0);
        idx.push_back(base + 2);
        idx.push_back(base + 3);
    };

    const Vec4 col{0.65f, 0.66f, 0.7f, 1.0f};
    addFace({0, 0, 1}, {-h, -h, h}, {h, -h, h}, {h, h, h}, {-h, h, h}, col);
    addFace({0, 0, -1}, {h, -h, -h}, {-h, -h, -h}, {-h, h, -h}, {h, h, -h}, col);
    addFace({0, 1, 0}, {-h, h, h}, {h, h, h}, {h, h, -h}, {-h, h, -h}, col);
    addFace({0, -1, 0}, {-h, -h, -h}, {h, -h, -h}, {h, -h, h}, {-h, -h, h}, col);
    addFace({1, 0, 0}, {h, -h, h}, {h, -h, -h}, {h, h, -h}, {h, h, h}, col);
    addFace({-1, 0, 0}, {-h, -h, -h}, {-h, -h, h}, {-h, h, h}, {-h, h, -h}, col);

    return create(v, idx);
}

GpuMesh MeshFactory::createPlane(float size, int subdivisions) {
    std::vector<Vertex> verts;
    std::vector<uint32_t> indices;
    const int n = std::max(1, subdivisions);
    const float step = size / static_cast<float>(n);
    const float origin = -size * 0.5f;

    for (int y = 0; y <= n; ++y) {
        for (int x = 0; x <= n; ++x) {
            Vertex vert{};
            vert.position = {origin + x * step, 0.0f, origin + y * step};
            vert.normal = {0, 1, 0};
            vert.uv = {static_cast<float>(x) / n, static_cast<float>(y) / n};
            vert.color = {0.45f, 0.47f, 0.5f, 1.0f};
            verts.push_back(vert);
        }
    }

    for (int y = 0; y < n; ++y) {
        for (int x = 0; x < n; ++x) {
            const uint32_t i0 = static_cast<uint32_t>(y * (n + 1) + x);
            const uint32_t i1 = i0 + 1;
            const uint32_t i2 = i0 + static_cast<uint32_t>(n + 1);
            const uint32_t i3 = i2 + 1;
            indices.push_back(i0);
            indices.push_back(i2);
            indices.push_back(i1);
            indices.push_back(i1);
            indices.push_back(i2);
            indices.push_back(i3);
        }
    }
    return create(verts, indices);
}

GpuMesh MeshFactory::createCapsule(float radius, float halfHeight, int segments) {
    // Approximate as a stretched UV sphere for graybox player.
    std::vector<Vertex> verts;
    std::vector<uint32_t> indices;
    const int stacks = segments;
    const int slices = segments * 2;

    for (int i = 0; i <= stacks; ++i) {
        const float v = static_cast<float>(i) / stacks;
        const float phi = v * 3.14159265f;
        for (int j = 0; j <= slices; ++j) {
            const float u = static_cast<float>(j) / slices;
            const float theta = u * 6.2831853f;
            Vec3 n{std::sin(phi) * std::cos(theta), std::cos(phi), std::sin(phi) * std::sin(theta)};
            Vec3 p = n * radius;
            p.y += (n.y >= 0.0f ? halfHeight : -halfHeight);
            verts.push_back({p, n, {u, v}, {0.75f, 0.35f, 0.3f, 1.0f}});
        }
    }

    for (int i = 0; i < stacks; ++i) {
        for (int j = 0; j < slices; ++j) {
            const uint32_t i0 = static_cast<uint32_t>(i * (slices + 1) + j);
            const uint32_t i1 = i0 + 1;
            const uint32_t i2 = i0 + static_cast<uint32_t>(slices + 1);
            const uint32_t i3 = i2 + 1;
            indices.push_back(i0);
            indices.push_back(i2);
            indices.push_back(i1);
            indices.push_back(i1);
            indices.push_back(i2);
            indices.push_back(i3);
        }
    }
    return create(verts, indices);
}

} // namespace nu
