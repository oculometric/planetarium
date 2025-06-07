#pragma once

#include <vulkan/vulkan.h>
#include <array>
#include <vector>
#include <string>

#include "reference_counter.h"
#include "vector3.h"
#include "vector2.h"

typedef PTCountedPointer<class PTBuffer_T> PTBuffer;

struct PTVertex
{
    PTVector3f position;
    PTVector3f colour;
    PTVector3f normal;
    PTVector3f tangent;
    PTVector2f uv;
};

class PTMesh_T
{
private:
    VkDevice device = VK_NULL_HANDLE;

    PTBuffer vertex_buffer;
    PTBuffer index_buffer;
    uint32_t vertex_count = 0;
    uint32_t index_count = 0;

    std::string origin_path;

public:
    PTMesh_T() = delete;
    PTMesh_T(const PTMesh_T& other) = delete;
    PTMesh_T(const PTMesh_T&& other) = delete;
    PTMesh_T operator=(const PTMesh_T& other) = delete;
    PTMesh_T operator=(const PTMesh_T&& other) = delete;
    ~PTMesh_T();

    static inline PTCountedPointer<PTMesh_T> createMesh(std::string mesh_path)
    { return PTCountedPointer<PTMesh_T>(new PTMesh_T(mesh_path)); }
    static inline PTCountedPointer<PTMesh_T> createMesh(std::vector<PTVertex> vertices, std::vector<uint16_t> indices)
    { return PTCountedPointer<PTMesh_T>(new PTMesh_T(vertices, indices)); }

    VkBuffer getVertexBuffer() const;
    VkBuffer getIndexBuffer() const;
    inline uint32_t getVertexCount() const { return vertex_count; }
    inline uint32_t getIndexCount() const { return index_count; }

    static VkVertexInputBindingDescription getVertexBindingDescription();
    static std::array<VkVertexInputAttributeDescription, 5> getVertexAttributeDescriptions();

private:
    PTMesh_T(std::string mesh_path);
    PTMesh_T(std::vector<PTVertex> vertices, std::vector<uint16_t> indices);

    static bool readFileToBuffers(std::string file_name, std::vector<PTVertex>& vertices, std::vector<uint16_t>& indices);
    void createVertexBuffers(const std::vector<PTVertex>& vertices, const std::vector<uint16_t>& indices);
};

typedef PTCountedPointer<PTMesh_T> PTMesh;
