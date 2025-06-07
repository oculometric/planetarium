#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include <string>
#include <map>

#include "reference_counter.h"

class PTShader_T
{
public:
    struct BindingInfo
    {
        std::string identifier;
        uint16_t bind_point = 0;
        uint32_t size = 0;
        VkDescriptorType type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    };

private:
    VkDevice device = VK_NULL_HANDLE;

    VkShaderModule vertex_shader = VK_NULL_HANDLE;
    VkShaderModule fragment_shader = VK_NULL_HANDLE;
    VkShaderModule geometry_shader = VK_NULL_HANDLE;

    VkDescriptorSetLayout descriptor_set_layout = VK_NULL_HANDLE;
    std::vector<BindingInfo> descriptor_bindings;

    std::string origin_path;
    bool geom_shader_present = false;
    
public:
    PTShader_T() = delete;
    PTShader_T(PTShader_T& other) = delete;
    PTShader_T(PTShader_T&& other) = delete;
    void operator=(PTShader_T& other) = delete;
    void operator=(PTShader_T&& other) = delete;
    ~PTShader_T();

    static inline PTCountedPointer<PTShader_T> createShader(std::string shader_path_stub, bool is_precompiled, bool has_geometry_shader = false)
    { return PTCountedPointer<PTShader_T>(new PTShader_T(shader_path_stub, is_precompiled, has_geometry_shader)); }

    std::vector<VkPipelineShaderStageCreateInfo> getStageCreateInfo() const;
    
    inline VkDescriptorSetLayout getDescriptorSetLayout() const { return descriptor_set_layout; }
    size_t getDescriptorCount() const;
    BindingInfo getDescriptorBinding(size_t index) const;
    bool hasDescriptorWithBinding(uint16_t binding, BindingInfo& out, size_t& index);

private:
    PTShader_T(std::string shader_path_stub, bool is_precompiled, bool has_geometry_shader = false);

    bool readRawAndCompile(std::string shader_path_stub, std::vector<char>& vertex_code, std::vector<char>& fragment_code, std::vector<char>& geometry_code);
    bool readPrecompiled(std::string shader_path_stub, std::vector<char>& vertex_code, std::vector<char>& fragment_code, std::vector<char>& geometry_code);
    void createShaderModules(const std::vector<char>& vertex_code, const std::vector<char>& fragment_code, std::vector<char>& geometry_code);
    void createDescriptorSetLayout();
    void insertDescriptor(BindingInfo descriptor);
};

typedef PTCountedPointer<PTShader_T> PTShader;
