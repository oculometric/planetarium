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

    PTBuffer* vertex_buffer = nullptr;
    PTBuffer* index_buffer = nullptr;
    uint32_t index_count = 0;

    std::string origin_path;

public:
    PTMesh() = delete;
    PTMesh(const PTMesh& other) = delete;
    PTMesh(const PTMesh&& other) = delete;
    PTMesh operator=(const PTMesh& other) = delete;
    PTMesh operator=(const PTMesh&& other) = delete;

    inline VkBuffer getVertexBuffer() const { return vertex_buffer->getBuffer(); }
    inline VkBuffer getIndexBuffer() const { return index_buffer->getBuffer(); }
    inline uint32_t getIndexCount() const { return index_count; }

    // TODO: functions to update the contents of the buffers

    static VkVertexInputBindingDescription getVertexBindingDescription();
    static std::array<VkVertexInputAttributeDescription, 5> getVertexAttributeDescriptions();

private:
    PTMesh(VkDevice _device, std::string mesh_path, const PTPhysicalDevice& physical_device);
    PTMesh(VkDevice _device, std::vector<PTVertex> vertices, std::vector<uint16_t> indices, const PTPhysicalDevice& physical_device);

    ~PTMesh();

    static void readFileToBuffers(std::string file_name, std::vector<PTVertex>& vertices, std::vector<uint16_t>& indices);
    void createVertexBuffers(const PTPhysicalDevice& physical_device, std::vector<PTVertex> vertices, std::vector<uint16_t> indices);
};