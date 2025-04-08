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
    
    descriptor_bindings.push_back(UniformDescriptor{ "TransformUniforms", TRANSFORM_UNIFORM_BINDING, sizeof(TransformUniforms) });
    descriptor_bindings.push_back(UniformDescriptor{ "SceneUniforms", SCENE_UNIFORM_BINDING, sizeof(SceneUniforms) });
    descriptor_bindings.push_back(UniformDescriptor{ "Test", 2, 256 }); // FIXME: remove this
	// TODO: reflect these into existence
    // TODO: check if any incoming descriptor bindings bind to 0, and discard them if so
    createDescriptorSetLayout();
}

vector<VkPipelineShaderStageCreateInfo> PTShader::getStageCreateInfo() const
{
    vector<VkPipelineShaderStageCreateInfo> infos;

    // vertex shader info
    VkPipelineShaderStageCreateInfo vert_stage_create_info{ };
    vert_stage_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vert_stage_create_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vert_stage_create_info.module = vertex_shader;
    vert_stage_create_info.pName = "main";

    // fragment shader info
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
    // destroy the layout and the blobs
    vkDestroyDescriptorSetLayout(device, descriptor_set_layout, nullptr);
    vkDestroyShaderModule(device, vertex_shader, nullptr);
    vkDestroyShaderModule(device, fragment_shader, nullptr);
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

    // TODO: load a text shader, compile it, and then load shader blobs from it (also, extract the descriptor set layout)
    
    // open vertex and frag shader files (precompiled)
    vert_file.open(shader_path_stub + "_vert.spv", ios::ate | ios::binary);
    if (!vert_file.is_open())
        throw runtime_error("unable to open vertex shader for " + shader_path_stub);
    frag_file.open(shader_path_stub + "_frag.spv", ios::ate | ios::binary);
    if (!frag_file.is_open())
    {
        vert_file.close();
        throw runtime_error("unable to open fragment shader for " + shader_path_stub);
    }

    // clear and resize the vertex and fragment buffers
    vertex_code.clear();
    fragment_code.clear();
    size_t vert_size = (size_t)vert_file.tellg();
    size_t frag_size = (size_t)frag_file.tellg();
    vertex_code.resize(vert_size);
    fragment_code.resize(frag_size);

    // read the entirety of both files
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
    // turn a block of bytes into vertex buffer
    VkShaderModuleCreateInfo vert_create_info{ };
    vert_create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    vert_create_info.codeSize = vertex_code.size();
    vert_create_info.pCode = reinterpret_cast<const uint32_t*>(vertex_code.data());

    if (vkCreateShaderModule(device, &vert_create_info, nullptr, &vertex_shader) != VK_SUCCESS)
        throw std::runtime_error("unable to create vertex shader module");

    // turn a block of bytes into a frag buffer
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

    // go through all the descriptor bindings which are listed (currently only uniform buffers are supported)
    // and create descriptor set layout bindings
    for (UniformDescriptor descriptor : descriptor_bindings)
    {
        VkDescriptorSetLayoutBinding binding{ };
        binding.binding = descriptor.bind_point;
        binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER; // TODO: implement texture descriptors
        binding.descriptorCount = 1;
        binding.stageFlags = VK_SHADER_STAGE_ALL_GRAPHICS;

        bindings.push_back(binding);
    }

    // create the descriptor set layout. i'm so so very tired of those three words
    VkDescriptorSetLayoutCreateInfo layout_create_info{ };
    layout_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layout_create_info.bindingCount = static_cast<uint32_t>(bindings.size());
    layout_create_info.pBindings = bindings.data();

    if (vkCreateDescriptorSetLayout(device, &layout_create_info, nullptr, &descriptor_set_layout) != VK_SUCCESS)
        throw runtime_error("unable to create descriptor set layout");
}
