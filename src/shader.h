#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include <string>

using namespace std;

class PTShader
{
private:
    VkShaderModule vertex_shader = VK_NULL_HANDLE;
    VkShaderModule fragment_shader = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;
public:
    PTShader(VkDevice _device, const string shader_path_stub);

    PTShader() = delete;
    PTShader(PTShader& other) = delete;
    PTShader(PTShader&& other) = delete;
    void operator=(PTShader& other) = delete;
    void operator=(PTShader&& other) = delete;

    vector<VkPipelineShaderStageCreateInfo> getStageCreateInfo();

    ~PTShader();
private:
    bool readFromFile(const string shader_path_stub, vector<char>& vertex_code, vector<char>& fragment_code);
    void createShaderModules(const vector<char>& vertex_code, const vector<char>& fragment_code);
    void destroyShaderModules();
};