#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include <string>

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

    // TODO: in shader, keep track of descriptor set layout: names of uniforms, offsets, and expected types (also default values). detected from shader
    // TODO: integrate [https://github.com/KhronosGroup/SPIRV-Reflect]
    // TODO: in fact, actually the shader should track the uniform layout (list of descriptor bindings, i.e. uniform variables), and we just read it and use it to store the `uniforms` into the `buffer`

    friend class PTResourceManager;
private:
    VkDevice device = VK_NULL_HANDLE;

    VkShaderModule vertex_shader = VK_NULL_HANDLE;
    VkShaderModule fragment_shader = VK_NULL_HANDLE;

    VkDescriptorSetLayout descriptor_set_layout = VK_NULL_HANDLE;

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

private:
    bool readFromFile(std::string shader_path_stub, std::vector<char>& vertex_code, std::vector<char>& fragment_code);
    void createShaderModules(const std::vector<char>& vertex_code, const std::vector<char>& fragment_code);
    void createDescriptorSetLayout();
    void destroyShaderModules();
};