#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include <string>
#include <map>

#include "resource.h"

class PTShader : public PTResource
{
public:
    enum UniformType
    {
        FLOAT,
        VEC2,
        VEC3,
        VEC4,
        INT,
        MAT3,
        MAT4,
        TEXTURE
    };

    struct UniformDescriptor
    {
        std::string identifier;
        uint16_t offset;
        uint16_t size;
        UniformType type;
    };

    friend class PTResourceManager;
private:
    VkDevice device = VK_NULL_HANDLE;

    VkShaderModule vertex_shader = VK_NULL_HANDLE;
    VkShaderModule fragment_shader = VK_NULL_HANDLE;

    VkDescriptorSetLayout descriptor_set_layout = VK_NULL_HANDLE;
    // TODO: shader needs to know the size of its descriptors (their bindings, offsets, and sizes)
    std::vector<std::pair<uint32_t, std::pair<VkDeviceSize, VkDeviceSize>>> descriptor_bindings_and_sizes;
    std::map<std::string, UniformDescriptor> uniform_variables;

    PTShader(VkDevice _device, std::string shader_path_stub);

    ~PTShader();
    
public:
    PTShader() = delete;
    PTShader(PTShader& other) = delete;
    PTShader(PTShader&& other) = delete;
    void operator=(PTShader& other) = delete;
    void operator=(PTShader&& other) = delete;

    std::vector<VkPipelineShaderStageCreateInfo> getStageCreateInfo() const;
    
    inline VkDescriptorSetLayout getDescriptorSetLayout() const { return descriptor_set_layout; }
    size_t getDescriptorCount() const;
    std::pair<uint32_t, std::pair<VkDeviceSize, VkDeviceSize>> getDescriptorBinding(size_t index) const;
    uint32_t getDescriptorSetSize() const;

private:
    bool readFromFile(std::string shader_path_stub, std::vector<char>& vertex_code, std::vector<char>& fragment_code);
    void createShaderModules(const std::vector<char>& vertex_code, const std::vector<char>& fragment_code);
    // TODO: shader should use [https://github.com/KhronosGroup/SPIRV-Reflect] to extract all the descriptor set layouts, their bindings and sizes, and also a map of their element names, types, sizes and offsets
    void createDescriptorSetLayout();
    void destroyShaderModules();
};