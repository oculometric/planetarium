#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include <string>
#include <map>

#include "resource.h"

class PTShader : public PTResource
{
public:
    struct UniformDescriptor
    {
        std::string identifier;
        uint16_t bind_point;
        uint32_t size;
    };

    friend class PTResourceManager;
private:
    VkDevice device = VK_NULL_HANDLE;

    VkShaderModule vertex_shader = VK_NULL_HANDLE;
    VkShaderModule fragment_shader = VK_NULL_HANDLE;

    VkDescriptorSetLayout descriptor_set_layout = VK_NULL_HANDLE;
    std::vector<UniformDescriptor> descriptor_bindings;

    std::string origin_path;
    
public:
    PTShader() = delete;
    PTShader(PTShader& other) = delete;
    PTShader(PTShader&& other) = delete;
    void operator=(PTShader& other) = delete;
    void operator=(PTShader&& other) = delete;

    std::vector<VkPipelineShaderStageCreateInfo> getStageCreateInfo() const;
    
    inline VkDescriptorSetLayout getDescriptorSetLayout() const { return descriptor_set_layout; }
    size_t getDescriptorCount() const;
    UniformDescriptor getDescriptorBinding(size_t index) const;

private:
    PTShader(VkDevice _device, std::string shader_path_stub);

    ~PTShader();

    bool readFromFile(std::string shader_path_stub, std::vector<char>& vertex_code, std::vector<char>& fragment_code);
    void createShaderModules(const std::vector<char>& vertex_code, const std::vector<char>& fragment_code);
    void createDescriptorSetLayout();
    bool hasDescriptorWithBinding(uint16_t binding, UniformDescriptor& out, size_t& index);
    void insertDescriptor(UniformDescriptor descriptor);
};