#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include <string>

class PTShader
{
private:
    VkDevice device = VK_NULL_HANDLE;

    VkShaderModule vertex_shader = VK_NULL_HANDLE;
    VkShaderModule fragment_shader = VK_NULL_HANDLE;

    VkDescriptorSetLayout descriptor_set_layout = VK_NULL_HANDLE;
public:
    PTShader(VkDevice _device, std::string shader_path_stub);

    PTShader() = delete;
    PTShader(PTShader& other) = delete;
    PTShader(PTShader&& other) = delete;
    void operator=(PTShader& other) = delete;
    void operator=(PTShader&& other) = delete;

    std::vector<VkPipelineShaderStageCreateInfo> getStageCreateInfo() const;
    inline VkDescriptorSetLayout getDescriptorSetLayout() const { return descriptor_set_layout; }

    ~PTShader();
private:
    bool readFromFile(std::string shader_path_stub, std::vector<char>& vertex_code, std::vector<char>& fragment_code);
    void createShaderModules(const std::vector<char>& vertex_code, const std::vector<char>& fragment_code);
    void createDescriptorSetLayout();
    void destroyShaderModules();
};