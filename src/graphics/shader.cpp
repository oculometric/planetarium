#include "shader.h"

#include "constant.h"
#include <fstream>

#include "spirv_reflect.h"

using namespace std;

PTShader::PTShader(VkDevice _device, const string shader_path_stub)
{
    vector<char> vert, frag;
    device = _device;
    if (readFromFile(shader_path_stub, vert, frag))
        createShaderModules(vert, frag);
    
    descriptor_bindings.push_back(UniformDescriptor{ "Common", 0, sizeof(CommonUniforms) });
    descriptor_bindings.push_back(UniformDescriptor{ "Test", 1, 256 }); // FIXME: remove this
    // TODO: check if any incoming descriptor bindings bind to 0, and discard them if so
    createDescriptorSetLayout();
}

vector<VkPipelineShaderStageCreateInfo> PTShader::getStageCreateInfo() const
{
    vector<VkPipelineShaderStageCreateInfo> infos;

    VkPipelineShaderStageCreateInfo vert_stage_create_info{ };
    vert_stage_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vert_stage_create_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vert_stage_create_info.module = vertex_shader;
    vert_stage_create_info.pName = "main";

    VkPipelineShaderStageCreateInfo frag_stage_create_info{ };
    frag_stage_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    frag_stage_create_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    frag_stage_create_info.module = fragment_shader;
    frag_stage_create_info.pName = "main";

    infos.push_back(vert_stage_create_info);
    infos.push_back(frag_stage_create_info);

    return infos;
}

PTShader::~PTShader()
{
    vkDestroyDescriptorSetLayout(device, descriptor_set_layout, nullptr);
    destroyShaderModules();
}

size_t PTShader::getDescriptorCount() const
{
    return descriptor_bindings.size();
}

PTShader::UniformDescriptor PTShader::getDescriptorBinding(size_t index) const
{
    return descriptor_bindings[index];
}

bool PTShader::readFromFile(const string shader_path_stub, vector<char>& vertex_code, vector<char>& fragment_code)
{
    ifstream vert_file, frag_file;

    vert_file.open(shader_path_stub + "_vert.spv", ios::ate | ios::binary);
    if (!vert_file.is_open())
        throw runtime_error("unable to open vertex shader for " + shader_path_stub);
    frag_file.open(shader_path_stub + "_frag.spv", ios::ate | ios::binary);
    if (!frag_file.is_open())
    {
        vert_file.close();
        throw runtime_error("unable to open fragment shader for " + shader_path_stub);
    }

    vertex_code.clear();
    fragment_code.clear();
    size_t vert_size = (size_t)vert_file.tellg();
    size_t frag_size = (size_t)frag_file.tellg();
    vertex_code.resize(vert_size);
    fragment_code.resize(frag_size);

    vert_file.seekg(0);
    frag_file.seekg(0);

    vert_file.read(vertex_code.data(), vert_size);
    frag_file.read(fragment_code.data(), frag_size);

    vert_file.close();
    frag_file.close();

    return true;
}

void PTShader::createShaderModules(const vector<char>& vertex_code, const vector<char>& fragment_code)
{
    VkShaderModuleCreateInfo vert_create_info{ };
    vert_create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    vert_create_info.codeSize = vertex_code.size();
    vert_create_info.pCode = reinterpret_cast<const uint32_t*>(vertex_code.data());

    if (vkCreateShaderModule(device, &vert_create_info, nullptr, &vertex_shader) != VK_SUCCESS)
        throw std::runtime_error("unable to create vertex shader module");

    VkShaderModuleCreateInfo frag_create_info{ };
    frag_create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    frag_create_info.codeSize = fragment_code.size();
    frag_create_info.pCode = reinterpret_cast<const uint32_t*>(fragment_code.data());

    if (vkCreateShaderModule(device, &frag_create_info, nullptr, &fragment_shader) != VK_SUCCESS)
        throw std::runtime_error("unable to create fragment shader module");
}

void PTShader::createDescriptorSetLayout()
{
    vector<VkDescriptorSetLayoutBinding> bindings = { };

    for (UniformDescriptor descriptor : descriptor_bindings)
    {
        VkDescriptorSetLayoutBinding binding{ };
        binding.binding = descriptor.bind_point;
        binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER; // TODO: implement texture descriptors
        binding.descriptorCount = 1;
        binding.stageFlags = VK_SHADER_STAGE_ALL_GRAPHICS;

        bindings.push_back(binding);
    }

    VkDescriptorSetLayoutCreateInfo layout_create_info{ };
    layout_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layout_create_info.bindingCount = static_cast<uint32_t>(bindings.size());
    layout_create_info.pBindings = bindings.data();

    if (vkCreateDescriptorSetLayout(device, &layout_create_info, nullptr, &descriptor_set_layout) != VK_SUCCESS)
        throw runtime_error("unable to create descriptor set layout");
}

void PTShader::destroyShaderModules()
{
    vkDestroyShaderModule(device, vertex_shader, nullptr);
    vkDestroyShaderModule(device, fragment_shader, nullptr);
}
