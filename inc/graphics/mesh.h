#pragma once

#include <vulkan/vulkan.h>
#include <array>
#include <vector>
#include <string>

#include "resource.h"
#include "physical_device.h"
#include "buffer.h"
#include "vector3.h"
#include "vector2.h"

struct PTVertex
{
    PTVector3f position;
    PTVector3f colour;
    PTVector3f normal;
    PTVector3f tangent;
    PTVector2f uv;
};

class PTMesh : public PTResource
{
    friend class PTResourceManager;
private:
    VkDevice device = VK_NULL_HANDLE;

    PTBuffer vertex_buffer;
    PTBuffer index_buffer;
    uint32_t vertex_count = 0;
    uint32_t index_count = 0;

    std::string origin_path;

public:
    PTMesh() = delete;
    PTMesh(const PTMesh& other) = delete;
    PTMesh(const PTMesh&& other) = delete;
    PTMesh operator=(const PTMesh& other) = delete;
    PTMesh operator=(const PTMesh&& other) = delete;

    inline VkBuffer getVertexBuffer() const { return vertex_buffer.getPointer() != nullptr ? vertex_buffer->getBuffer() : VK_NULL_HANDLE; }
    inline VkBuffer getIndexBuffer() const { return index_buffer.getPointer() != nullptr ? index_buffer->getBuffer() : VK_NULL_HANDLE; }
    inline uint32_t getVertexCount() const { return vertex_count; }
    inline uint32_t getIndexCount() const { return index_count; }

    static VkVertexInputBindingDescription getVertexBindingDescription();
    static std::array<VkVertexInputAttributeDescription, 5> getVertexAttributeDescriptions();

private:
    PTMesh(VkDevice _device, std::string mesh_path, const PTPhysicalDevice& physical_device);
    PTMesh(VkDevice _device, std::vector<PTVertex> vertices, std::vector<uint16_t> indices, const PTPhysicalDevice& physical_device);

    ~PTMesh();

    static bool readFileToBuffers(std::string file_name, std::vector<PTVertex>& vertices, std::vector<uint16_t>& indices);
    void createVertexBuffers(const PTPhysicalDevice& physical_device, const std::vector<PTVertex>& vertices, const std::vector<uint16_t>& indices);
};