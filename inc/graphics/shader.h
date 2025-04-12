#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include <string>
#include <map>

#include "resource.h"

class PTShader : public PTResource
{
public:
    struct BindingInfo
    {
        std::string identifier;
        uint16_t bind_point;
        uint32_t size;
        VkDescriptorType type;
    };

    friend class PTResourceManager;
private:
    VkDevice device = VK_NULL_HANDLE;

    VkShaderModule vertex_shader = VK_NULL_HANDLE;
    VkShaderModule fragment_shader = VK_NULL_HANDLE;

    VkDescriptorSetLayout descriptor_set_layout = VK_NULL_HANDLE;
    std::vector<BindingInfo> descriptor_bindings;

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
    BindingInfo getDescriptorBinding(size_t index) const;
    bool hasDescriptorWithBinding(uint16_t binding, BindingInfo& out, size_t& index);

private:
    PTShader(VkDevice _device, std::string shader_path_stub, bool is_precompiled);

    ~PTShader();

    bool readRawAndCompile(std::string shader_path_stub, std::vector<char>& vertex_code, std::vector<char>& fragment_code);
    bool readPrecompiled(std::string shader_path_stub, std::vector<char>& vertex_code, std::vector<char>& fragment_code);
    void createShaderModules(const std::vector<char>& vertex_code, const std::vector<char>& fragment_code);
    void createDescriptorSetLayout();
    void insertDescriptor(BindingInfo descriptor);
};