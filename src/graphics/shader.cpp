#include "shader.h"

#include "constant.h"
#include <fstream>

#include "spirv_reflect.h"

using namespace std;

PTShader::PTShader(VkDevice _device, const string shader_path_stub, bool is_precompiled, bool has_geometry_shader)
{
    origin_path = shader_path_stub;
    geom_shader_present = has_geometry_shader;

    vector<char> vert, frag, geom;
    device = _device;
    if (is_precompiled)
    {
        if (readPrecompiled(shader_path_stub, vert, frag, geom))
            createShaderModules(vert, frag, geom);
        else
        {
            debugLog("ERROR: failed to load precompiled shader " + shader_path_stub);
            geom_shader_present = false;
            if (!readRawAndCompile(DEFAULT_SHADER_PATH, vert, frag, geom))
                throw runtime_error("failed to load default shader");
            else
                createShaderModules(vert, frag, geom);
        }
    }
    else
    {
        if (readRawAndCompile(shader_path_stub, vert, frag, geom))
            createShaderModules(vert, frag, geom);
        else
        {
            debugLog("ERROR: failed to compile shader " + shader_path_stub);
            geom_shader_present = false;
            if (!readRawAndCompile(DEFAULT_SHADER_PATH, vert, frag, geom))
                throw runtime_error("failed to load default shader");
            else
                createShaderModules(vert, frag, geom);
        }
    }

    // these bindings should always be binding 0 and 1, and should always be present
    descriptor_bindings.push_back(BindingInfo{ "TransformUniforms", TRANSFORM_UNIFORM_BINDING, sizeof(TransformUniforms), VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER });
    descriptor_bindings.push_back(BindingInfo{ "SceneUniforms", SCENE_UNIFORM_BINDING, sizeof(SceneUniforms), VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER });
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

    if (geom_shader_present)
    {
        // geometry shader info
        VkPipelineShaderStageCreateInfo geom_stage_create_info{ };
        geom_stage_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        geom_stage_create_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        geom_stage_create_info.module = geometry_shader;
        geom_stage_create_info.pName = "main";
        
        infos.push_back(geom_stage_create_info);
    }

    return infos;
}

PTShader::~PTShader()
{
    // destroy the layout and the blobs
    vkDestroyDescriptorSetLayout(device, descriptor_set_layout, nullptr);
    vkDestroyShaderModule(device, vertex_shader, nullptr);
    vkDestroyShaderModule(device, fragment_shader, nullptr);
    if (geom_shader_present)
        vkDestroyShaderModule(device, geometry_shader, nullptr);
}

size_t PTShader::getDescriptorCount() const
{
    return descriptor_bindings.size();
}

PTShader::BindingInfo PTShader::getDescriptorBinding(size_t index) const
{
    return descriptor_bindings[index];
}

bool PTShader::hasDescriptorWithBinding(uint16_t binding, BindingInfo& out, size_t& index)
{
    index = 0;
    for (BindingInfo descriptor : descriptor_bindings)
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

bool PTShader::readRawAndCompile(string shader_path_stub, vector<char>& vertex_code, vector<char>& fragment_code, vector<char>& geometry_code)
{
    // run compile commands
    string compiler = "";
    string deleter = "";
    string windows_path_stub = shader_path_stub;
    string out_path_base = shader_path_stub + "_TEMP_" + to_string((uint32_t)((size_t)this));
    string windows_out_base = out_path_base;
#ifdef _WIN32
    compiler = "glslc";
    deleter = "del";
    for (size_t i = 0; i < windows_path_stub.size(); i++)
    {
        if (windows_path_stub[i] == '/')
            windows_path_stub[i] = '\\';
    }
    for (size_t i = 0; i < windows_out_base.size(); i++)
    {
        if (windows_out_base[i] == '/')
            windows_out_base[i] = '\\';
    }
#else
    compiler = "glslc";
    deleter = "rm";
#endif

    string command_out;
    string command;
    int result;
    
    // try to compile the vertex shader
    command = compiler + ' ' + shader_path_stub + ".vert -o " + out_path_base + "_vert.spv";
    result = exec(command.c_str(), command_out);

    // if failed, report error
    if (result != 0)
    {
        debugLog("WARNING: failed to compile " + shader_path_stub + ".vert:");
        debugLog(command_out);
        return false;
    }

    // try to compile the fragment shader
    command = compiler + ' ' + shader_path_stub + ".frag -o " + out_path_base + "_frag.spv";
    result = exec(command.c_str(), command_out);

    // if failed, delete the vertex shader and report error
    if (result != 0)
    {
        debugLog("WARNING: failed to compile " + shader_path_stub + ".frag:");
        debugLog(command_out);

        command = deleter + ' ' + windows_out_base + "_vert.spv";
        system(command.c_str());
        return false;
    }

    // try to compile the geometry shader
    if (geom_shader_present)
    {
        command = compiler + ' ' + shader_path_stub + ".geom -o " + out_path_base + "_geom.spv";
        result = exec(command.c_str(), command_out);

        // if failed, delete the vertex and fragment shaders and report error
        if (result != 0)
        {
            debugLog("WARNING: failed to compile " + shader_path_stub + ".geom:");
            debugLog(command_out);

            command = deleter + ' ' + windows_out_base + "_vert.spv";
            system(command.c_str());
            command = deleter + ' ' + windows_out_base + "_frag.spv";
            system(command.c_str());
            return false;
        }
    }
    
    // load shader modules using the other function
    bool load_result = readPrecompiled(out_path_base, vertex_code, fragment_code, geometry_code);
    
    // delete the generated files
    command = deleter + ' ' + windows_out_base + "_vert.spv";
    system(command.c_str());
    command = deleter + ' ' + windows_out_base + "_frag.spv";
    system(command.c_str());
    if (geom_shader_present)
    {
        command = deleter + ' ' + windows_out_base + "_geom.spv";
        system(command.c_str());
    }

    return load_result;
}

bool PTShader::readPrecompiled(const string shader_path_stub, vector<char>& vertex_code, vector<char>& fragment_code, vector<char>& geometry_code)
{
    ifstream vert_file, frag_file, geom_file;
    
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
    if (geom_shader_present)
    {
        geom_file.open(shader_path_stub + "_geom.spv", ios::ate | ios::binary);
        if (!geom_file.is_open())
        {
            vert_file.close();
            frag_file.close();
            debugLog("WARNING: unable to open " + shader_path_stub + "_geom.spv");
            return false;
        }
    }

    // clear and resize the vertex and fragment arrays, then read the whole of each file
    vertex_code.clear();
    size_t vert_size = (size_t)vert_file.tellg();
    vertex_code.resize(vert_size);

    vert_file.seekg(0);
    vert_file.read(vertex_code.data(), vert_size);
    vert_file.close();

    fragment_code.clear();
    size_t frag_size = (size_t)frag_file.tellg();
    fragment_code.resize(frag_size);

    frag_file.seekg(0);
    frag_file.read(fragment_code.data(), frag_size);
    frag_file.close();

    if (geom_shader_present)
    {
        geometry_code.clear();
        size_t geom_size = (size_t)geom_file.tellg();
        geometry_code.resize(geom_size);

        geom_file.seekg(0);
        geom_file.read(geometry_code.data(), geom_size);
        geom_file.close();
    }

    return true;
}

void PTShader::createShaderModules(const vector<char>& vertex_code, const vector<char>& fragment_code, std::vector<char>& geometry_code)
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
        BindingInfo descriptor{ binding.name, static_cast<uint16_t>(binding.binding), binding.block.padded_size };
        if (binding.descriptor_type == SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
            descriptor.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        else if (binding.descriptor_type == SPV_REFLECT_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
            descriptor.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        else
        {
            debugLog("ERROR: vertex shader '" + origin_path + "' contains unsupported descriptor of type " + to_string(binding.descriptor_type));
            continue;
        }
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
        BindingInfo descriptor{ binding.name, static_cast<uint16_t>(binding.binding), binding.block.padded_size };
        if (binding.descriptor_type == SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
            descriptor.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        else if (binding.descriptor_type == SPV_REFLECT_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
            descriptor.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        else
        {
            debugLog("ERROR: fragment shader '" + origin_path + "' contains unsupported descriptor of type " + to_string(binding.descriptor_type));
            continue;
        }
        insertDescriptor(descriptor);
    }
    spvReflectDestroyShaderModule(&reflect);

    if (geom_shader_present)
    {
        // turn a block of bytes into a geom buffer
        VkShaderModuleCreateInfo geom_create_info{ };
        geom_create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        geom_create_info.codeSize = geometry_code.size();
        geom_create_info.pCode = reinterpret_cast<const uint32_t*>(geometry_code.data());

        if (vkCreateShaderModule(device, &geom_create_info, nullptr, &geometry_shader) != VK_SUCCESS)
            throw runtime_error("unable to create geometry shader module");

        // extract a list of descriptor bindings from the geometry shader
        spvReflectCreateShaderModule(geometry_code.size(), geometry_code.data(), &reflect);
        for (size_t i = 0; i < reflect.descriptor_binding_count; i++)
        {
            SpvReflectDescriptorBinding binding = reflect.descriptor_bindings[i];
            BindingInfo descriptor{ binding.name, static_cast<uint16_t>(binding.binding), binding.block.padded_size };
            if (binding.descriptor_type == SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
                descriptor.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            else if (binding.descriptor_type == SPV_REFLECT_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
                descriptor.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            else
            {
                debugLog("ERROR: geometry shader '" + origin_path + "' contains unsupported descriptor of type " + to_string(binding.descriptor_type));
                continue;
            }
            insertDescriptor(descriptor);
        }
        spvReflectDestroyShaderModule(&reflect);
    }
}

void PTShader::createDescriptorSetLayout()
{
    vector<VkDescriptorSetLayoutBinding> bindings = { };

    // go through all the descriptor bindings which are listed (currently only uniform buffers are supported)
    // and create descriptor set layout bindings
    for (BindingInfo descriptor : descriptor_bindings)
    {
        VkDescriptorSetLayoutBinding binding{ };
        binding.binding = descriptor.bind_point;
        binding.descriptorType = descriptor.type;
        binding.descriptorCount = 1;
        binding.stageFlags = VK_SHADER_STAGE_ALL_GRAPHICS; // TODO: detect which stages it's used in and set this to something other than 'absoultely everything'

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

void PTShader::insertDescriptor(BindingInfo descriptor)
{
    if (descriptor.bind_point == TRANSFORM_UNIFORM_BINDING
     || descriptor.bind_point == SCENE_UNIFORM_BINDING)
        return;
    BindingInfo existing;
    size_t index;
    // handle duplicate bindings - either merge them if they're the same type, or error and do nothing if they're different types (e.g. a texture being bound to the same slot that a uniform was bound to previously)
    if (hasDescriptorWithBinding(descriptor.bind_point, existing, index))
    {
        if (descriptor.type != existing.type)
        {
            debugLog("ERROR: during shader " + origin_path + " loading, multiple descriptors found bound to " + to_string(descriptor.bind_point) + ", with incompatible types. the later one will be ignored");
            return;
        }
        debugLog("WARNING: during shader " + origin_path + " loading, multiple descriptors found bound to " + to_string(descriptor.bind_point) + ". i will overwrite with the larger one");
        if (existing.size < descriptor.size)
            descriptor_bindings[index] = descriptor;

        return;
    }
    descriptor_bindings.push_back(descriptor);
}
