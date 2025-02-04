#pragma once

#include <vulkan/vulkan.h>
#include <array>
#include <vector>
#include <string>

#include "physical_device.h"
#include "buffer.h"
#include "../lib/oculib/vector3.h"

struct PTVertex
{ // TODO: add normal, tangent, uv
    OLVector3f position;
    OLVector3f colour;
};

class PTMesh
{
private:
    VkDevice device = VK_NULL_HANDLE;

    PTBuffer* vertex_buffer = nullptr;
    PTBuffer* index_buffer = nullptr;
    uint32_t index_count = 0;

public:
    PTMesh() = delete;
    PTMesh(const PTMesh& other) = delete;
    PTMesh(const PTMesh&& other) = delete;
    PTMesh operator=(const PTMesh& other) = delete;
    PTMesh operator=(const PTMesh&& other) = delete;

    PTMesh(VkDevice _device, const PTPhysicalDevice& physical_device, std::string file_name);
    PTMesh(VkDevice _device, const PTPhysicalDevice& physical_device, std::vector<PTVertex> vertices, std::vector<uint16_t> indices);

    inline VkBuffer getVertexBuffer() const { return vertex_buffer->getBuffer(); }
    inline VkBuffer getIndexBuffer() const { return index_buffer->getBuffer(); }
    inline uint32_t getIndexCount() const { return index_count; }

    // TODO: functions to update the contents of the buffers

    static VkVertexInputBindingDescription getVertexBindingDescription();
    static std::array<VkVertexInputAttributeDescription, 2> getVertexAttributeDescriptions();

    ~PTMesh();

private:
    // TODO: proper file reading (replace oculib)
    static void readFileToBuffers(std::string file_name, std::vector<PTVertex>& vertices, std::vector<uint16_t>& indices);
    void createVertexBuffers(const PTPhysicalDevice& physical_device, std::vector<PTVertex> vertices, std::vector<uint16_t> indices);
};