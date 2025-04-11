#include "shader.h"

#include "constant.h"
#include <fstream>

#include "spirv_reflect.h"

using namespace std;

PTShader::PTShader(VkDevice _device, const string shader_path_stub, bool is_precompiled)
{
    origin_path = shader_path_stub;

    vector<char> vert, frag;
    device = _device;
    if (is_precompiled)
    {
        if (readPrecompiled(shader_path_stub, vert, frag))
            createShaderModules(vert, frag);
        else
        {
            debugLog("ERROR: failed to load precompiled shader " + shader_path_stub);
            if (!readRawAndCompile(DEFAULT_SHADER_PATH, vert, frag))
                throw runtime_error("failed to load default shader");
            else
                createShaderModules(vert, frag);
        }
    }
    else
    {
        if (readRawAndCompile(shader_path_stub, vert, frag))
            createShaderModules(vert, frag);
        else
        {
            debugLog("ERROR: failed to compile shader " + shader_path_stub);
            if (!readRawAndCompile(DEFAULT_SHADER_PATH, vert, frag))
                throw runtime_error("failed to load default shader");
            else
                createShaderModules(vert, frag);
        }
    }

    // these bindings should always be binding 0 and 1, and should always be present
    descriptor_bindings.push_back(UniformDescriptor{ "TransformUniforms", TRANSFORM_UNIFORM_BINDING, sizeof(TransformUniforms) });
    descriptor_bindings.push_back(UniformDescriptor{ "SceneUniforms", SCENE_UNIFORM_BINDING, sizeof(SceneUniforms) });
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

bool PTShader::readRawAndCompile(string shader_path_stub, vector<char>& vertex_code, vector<char>& fragment_code)
{
    // run compile commands
    string compiler = "";
#ifdef _WIN32
    compiler = "glslc.exe";
#else
    compiler = "glslc";
#endif

    string out_path_base = shader_path_stub + "_TEMP_" + to_string((uint32_t)((size_t)this));

    string command = compiler + ' ' + shader_path_stub + ".vert -o " + out_path_base + "_vert.spv";

    int result = system(command.c_str());
    if (result != 0)
    {
        debugLog("WARNING: failed to compile " + shader_path_stub + ".vert");
        return false;
    }

    command = compiler + ' ' + shader_path_stub + ".frag -o " + out_path_base + "_frag.spv";

    result = system(command.c_str());
    command = "rm " + out_path_base + "_vert.spv";
    if (result != 0)
    {
        debugLog("WARNING: failed to compile " + shader_path_stub + ".frag");
        system(command.c_str());
        return false;
    }
    
    // load shader modules using the other function
    bool load_result = readPrecompiled(out_path_base, vertex_code, fragment_code);
    
    // delete the generated files
    system(command.c_str());
    command = "rm " + out_path_base + "_frag.spv";
    system(command.c_str());

    return load_result;
}

bool PTShader::readPrecompiled(const string shader_path_stub, vector<char>& vertex_code, vector<char>& fragment_code)
{
    ifstream vert_file, frag_file;
    
    // open vertex and frag shader files (precompiled)
    vert_file.open(shader_path_stub + "_vert.spv", ios::ate | ios::binary);
    if (!vert_file.is_open())
    {
        debugLog("WARNING: unable to open " + shader_path_stub + "_vert.spv");
        return false;
    }
    frag_file.open(shader_path_stub + "_frag.spv", ios::ate | ios::binary);
    if (!frag_file.is_open())
    {
        vert_file.close();
        debugLog("WARNING: unable to open " + shader_path_stub + "_frag.spv");
        return false;
    }

    // clear and resize the vertex and fragment arrays
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
        throw runtime_error("unable to create vertex shader module");

    // extract a list of descriptor bindings from the vertex shader
    SpvReflectShaderModule reflect;
    spvReflectCreateShaderModule(vertex_code.size(), vertex_code.data(), &reflect);
    for (size_t i = 0; i < reflect.descriptor_binding_count; i++)
    {
        SpvReflectDescriptorBinding binding = reflect.descriptor_bindings[i];
        UniformDescriptor descriptor{ binding.name, static_cast<uint16_t>(binding.binding), binding.block.padded_size };
        insertDescriptor(descriptor);
    }
    spvReflectDestroyShaderModule(&reflect);

    // turn a block of bytes into a frag buffer
    VkShaderModuleCreateInfo frag_create_info{ };
    frag_create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    frag_create_info.codeSize = fragment_code.size();
    frag_create_info.pCode = reinterpret_cast<const uint32_t*>(fragment_code.data());

    if (vkCreateShaderModule(device, &frag_create_info, nullptr, &fragment_shader) != VK_SUCCESS)
        throw runtime_error("unable to create fragment shader module");

    // extract a list of descriptor bindings from the fragment shader
    spvReflectCreateShaderModule(fragment_code.size(), fragment_code.data(), &reflect);
    for (size_t i = 0; i < reflect.descriptor_binding_count; i++)
    {
        SpvReflectDescriptorBinding binding = reflect.descriptor_bindings[i];
        UniformDescriptor descriptor{ binding.name, static_cast<uint16_t>(binding.binding), binding.block.padded_size };
        insertDescriptor(descriptor);
    }
    spvReflectDestroyShaderModule(&reflect);
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

bool PTShader::hasDescriptorWithBinding(uint16_t binding, UniformDescriptor& out, size_t& index)
{
    index = 0;
    for (UniformDescriptor descriptor : descriptor_bindings)
    {
        if (descriptor.bind_point == binding)
        {
            out = descriptor;
            return true;
        }
        index++;
    }
    return false;
}

void PTShader::insertDescriptor(UniformDescriptor descriptor)
{
    if (descriptor.bind_point == TRANSFORM_UNIFORM_BINDING
     || descriptor.bind_point == SCENE_UNIFORM_BINDING)
        return;
    UniformDescriptor existing;
    size_t index;
    if (hasDescriptorWithBinding(descriptor.bind_point, existing, index))
    {
        // TODO: check that they are the same type here (texture vs uniform, throw error if they aren't)
        debugLog("WARNING: during shader " + string("NONAME") + " loading, multiple descriptors found bound to " + to_string(descriptor.bind_point) + ". i will overwrite with the larger one");
        if (existing.size < descriptor.size)
            descriptor_bindings[index] = descriptor;

        return;
    }
    descriptor_bindings.push_back(descriptor);
}
