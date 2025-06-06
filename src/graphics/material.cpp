#include "material.h"

#include <assert.h>
#include <fstream>
#include <cstring>

#include "resource_manager.h"
#include "shader.h"
#include "buffer.h"
#include "pipeline.h"
#include "image.h"
#include "swapchain.h"
#include "deserialiser.h"
#include "sampler.h"

using namespace std;

static map<string, VkCompareOp> depth_ops =
{
    { "NEVER", VK_COMPARE_OP_NEVER },
    { "LESS", VK_COMPARE_OP_LESS },
    { "EQUAL", VK_COMPARE_OP_EQUAL },
    { "LESS_OR_EQUAL", VK_COMPARE_OP_LESS_OR_EQUAL },
    { "GREATER", VK_COMPARE_OP_GREATER },
    { "NOT_EQUAL", VK_COMPARE_OP_NOT_EQUAL },
    { "GREATER_OR_EQUAL", VK_COMPARE_OP_GREATER_OR_EQUAL },
    { "ALWAYS", VK_COMPARE_OP_ALWAYS }
};

static map<string, VkCullModeFlags> cull_modes =
{
    { "BACK", VK_CULL_MODE_BACK_BIT },
    { "FRONT", VK_CULL_MODE_FRONT_BIT },
    { "NONE", VK_CULL_MODE_NONE },
    { "BOTH", VK_CULL_MODE_FRONT_AND_BACK }
};

static map<string, VkPolygonMode> polygon_modes =
{
    { "FILL", VK_POLYGON_MODE_FILL },
    { "LINE", VK_POLYGON_MODE_LINE },
    { "POINT", VK_POLYGON_MODE_POINT }
};

static map<string, VkSamplerAddressMode> repeat_modes = 
{
    { "REPEAT", VK_SAMPLER_ADDRESS_MODE_REPEAT },
    { "MIRROR", VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT },
    { "CLAMP", VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE },
    { "BORDER", VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER }
};

static map<string, VkFilter> filters = 
{
    { "LINEAR", VK_FILTER_LINEAR },
    { "NEAREST", VK_FILTER_NEAREST },
    { "CUBIC", VK_FILTER_CUBIC_IMG }
};

PTMaterial::PTMaterial(VkDevice _device, string material_path, PTRenderPass* _render_pass, PTSwapchain* swapchain)
{
    device = _device;
    origin_path = material_path;
    render_pass = _render_pass;
    origin_path = material_path;

    ifstream file(material_path, ios::ate);
    if (!file.is_open())
    {
        debugLog("ERROR: material file not found");

        file.open(DEFAULT_MATERIAL_PATH, ios::ate);
        if (!file.is_open())
            throw runtime_error("failed to load default material");
    }

    size_t size = file.tellg();
    string text;
    text.resize(size, ' ');
    file.seekg(0);
    file.read(text.data(), size);
    file.close();

    PTDeserialiser::MaterialParams params;
    vector<PTDeserialiser::UniformParam> uniforms;
    map<uint16_t, PTDeserialiser::TextureParam> _textures;
        
    PTDeserialiser::deserialiseMaterial(text, params, shader, uniforms, _textures);

    if (shader == nullptr)
        shader = PTResourceManager::get()->createShader(DEFAULT_SHADER_PATH, false);

    setPriority(params.priority);

    initialiseMaterial(swapchain, params.depth_write, params.depth_test, 
        depth_ops.contains(params.depth_op) ? depth_ops[params.depth_op] : VK_COMPARE_OP_LESS, 
        cull_modes.contains(params.culling) ? cull_modes[params.culling] : VK_CULL_MODE_BACK_BIT, 
        polygon_modes.contains(params.polygon_mode) ? polygon_modes[params.polygon_mode] : VK_POLYGON_MODE_FILL);

    shader->removeReferencer();

    for (PTDeserialiser::UniformParam variable : uniforms)
    {
        if (variable.binding == TRANSFORM_UNIFORM_BINDING
         || variable.binding == SCENE_UNIFORM_BINDING)
            continue;
        
        uint32_t buf_size = static_cast<uint32_t>(uniform_buffers[variable.binding]->getSize());
        uint8_t* target = (uint8_t*)uniform_buffers[variable.binding]->map();

        uint32_t data_size = static_cast<uint32_t>(variable.size);
        uint32_t data_offset = static_cast<uint32_t>(variable.offset);

        void* data;
        switch (variable.value.type)
        {
        case PTDeserialiser::ArgType::FLOAT_ARG: data = &(variable.value.f_val); break;
        case PTDeserialiser::ArgType::INT_ARG: data = &(variable.value.i_val); break;
        case PTDeserialiser::ArgType::VECTOR2_ARG: data = &(variable.value.v2_val); break;
        case PTDeserialiser::ArgType::VECTOR3_ARG: data = &(variable.value.v3_val); break;
        case PTDeserialiser::ArgType::VECTOR4_ARG: data = &(variable.value.v4_val); break;
        default: throw runtime_error("invalid argument type");
        }

        if (data_offset + data_size <= buf_size)
            memcpy(target + data_offset, data, data_size);
    }
    
    for (auto pair : _textures)
    {
        setTexture(pair.first, pair.second.texture,
            repeat_modes.contains(pair.second.repeat) ? repeat_modes[pair.second.repeat] : VK_SAMPLER_ADDRESS_MODE_REPEAT,
            filters.contains(pair.second.filter) ? filters[pair.second.filter] : VK_FILTER_LINEAR);
        pair.second.texture->removeReferencer();
    }
}

PTMaterial::PTMaterial(VkDevice _device, PTRenderPass* _render_pass, PTSwapchain* swapchain, PTShader* _shader, VkBool32 depth_write, VkBool32 depth_test, VkCompareOp depth_op, VkCullModeFlags culling, VkPolygonMode polygon_mode)
{
    device = _device;
    shader = _shader;
    render_pass = _render_pass;

    initialiseMaterial(swapchain, depth_write, depth_test, depth_op, culling, polygon_mode);
}

PTMaterial::~PTMaterial()
{
    for (auto pair : uniform_buffers)
    {
        removeDependency(pair.second);
    }

    for (auto pair : textures)
    {
        vkDestroyImageView(device, pair.second.second.first, nullptr);
        removeDependency(pair.second.first);
        removeDependency(pair.second.second.second);
    }

    removeDependency(render_pass);
    removeDependency(shader);
    removeDependency(pipeline);
}

void PTMaterial::applySetWrites(VkDescriptorSet descriptor_set)
{
    vector<VkWriteDescriptorSet> set_writes;
    size_t descriptors = getShader()->getDescriptorCount();
    vector< VkDescriptorBufferInfo> buffer_infos;
    buffer_infos.reserve(descriptors);
    for (size_t b = 0; b < descriptors; b++)
    {
        // get the binding and check if it's one of the engine bindings
        auto binding_info = getShader()->getDescriptorBinding(b);
        if (binding_info.bind_point == TRANSFORM_UNIFORM_BINDING
		 || binding_info.bind_point == SCENE_UNIFORM_BINDING)
			continue;

        // only create a write if it's a uniform buffer
        if (binding_info.type != VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
            continue;

        // configure a write operation which hooks the material uniform buffer up to the descriptor set
        VkDescriptorBufferInfo buffer_info{ };
        buffer_info.buffer = uniform_buffers[binding_info.bind_point]->getBuffer();
        buffer_info.offset = 0;
        buffer_info.range = binding_info.size;
        buffer_infos.push_back(buffer_info);

        VkWriteDescriptorSet write_set{ };
        write_set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write_set.dstSet = descriptor_set;
        write_set.dstBinding = binding_info.bind_point;
        write_set.dstArrayElement = 0;
        write_set.descriptorCount = 1;
        write_set.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        write_set.pBufferInfo = &(buffer_infos[buffer_infos.size() - 1]);
        set_writes.push_back(write_set);
    }

    vector<VkDescriptorImageInfo> image_infos;
    image_infos.reserve(textures.size());
    for (auto pair : textures)
    {
        // configure a write operation which hooks the material texture up to the descriptor set
        VkDescriptorImageInfo image_info{ };
        image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        image_info.imageView = pair.second.second.first;
        image_info.sampler = pair.second.second.second->getSampler();
        image_infos.push_back(image_info);

        VkWriteDescriptorSet write_set{ };
        write_set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write_set.dstSet = descriptor_set;
        write_set.dstBinding = pair.first;
        write_set.dstArrayElement = 0;
        write_set.descriptorCount = 1;
        write_set.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        write_set.pImageInfo = image_infos.data() + (image_infos.size() - 1);
        set_writes.push_back(write_set);
    }

    vkUpdateDescriptorSets(device, static_cast<uint32_t>(set_writes.size()), set_writes.data(), 0, nullptr);
}

void PTMaterial::setTexture(uint16_t bind_point, PTImage* texture, VkSamplerAddressMode repeat_mode, VkFilter filtering, VkImageAspectFlags aspect)
{
    PTShader::BindingInfo binding;
    size_t _;
    if (!getShader()->hasDescriptorWithBinding(bind_point, binding, _))
    {
        debugLog("WARNING: attempt to write to nonexistent texture binding on material '" + origin_path + "', ignoring");
        return;
    }
    if (binding.type != VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
    {
        debugLog("WARNING: attempt to write to non-texture descriptor binding on material '" + origin_path + "', ignoring");
        return;
    }

    // if it's the same texture that's already bound, do nothing
    if (texture == textures[bind_point].first && texture != nullptr)
        return;

    // if a texture is already bound, destroy it
    auto it = textures.find(bind_point);
    if (it != textures.end() && it->second.first != nullptr)
    {
        vkDestroyImageView(device, it->second.second.first, nullptr);
        removeDependency(it->second.first);
        removeDependency(it->second.second.second);
    }

    if (texture != nullptr)
    {
        addDependency(texture);
        textures[bind_point] = { texture, 
        {
            texture->createImageView(aspect),
            PTResourceManager::get()->createSampler(repeat_mode, filtering, filtering, 2)
        } };
        addDependency(textures[bind_point].second.second, false);
    }
    else
    {
        // if the texture is null, use the blank default image
        PTImage* img = PTResourceManager::get()->createImage(DEFAULT_TEXTURE_PATH);
        setTexture(bind_point, img);
        img->removeReferencer();
    }
    needs_texture_update = true;
}

PTImage* PTMaterial::getTexture(uint16_t bind_point)
{
    return textures[bind_point].first;
}

void PTMaterial::initialiseMaterial(PTSwapchain* swapchain, VkBool32 depth_write, VkBool32 depth_test, VkCompareOp depth_op, VkCullModeFlags culling, VkPolygonMode polygon_mode)
{
    pipeline = PTResourceManager::get()->createPipeline(shader, render_pass, swapchain, depth_write, depth_test, depth_op, culling, VK_FRONT_FACE_COUNTER_CLOCKWISE, polygon_mode, { });

    size_t descriptors = getShader()->getDescriptorCount();

    for (size_t b = 0; b < descriptors; b++)
    {
        // get the binding and check if it's one of the engine bindings
        auto binding_info = getShader()->getDescriptorBinding(b);
        if (binding_info.bind_point == TRANSFORM_UNIFORM_BINDING
            || binding_info.bind_point == SCENE_UNIFORM_BINDING)
            continue;

        // create a buffer if it's a uniform buffer, otherwise bind the blank texture
        if (binding_info.type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
        {
            PTBuffer* buffer = PTResourceManager::get()->createBuffer(binding_info.size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
            uniform_buffers[binding_info.bind_point] = buffer;
            addDependency(buffer, false);
        }
        else if (binding_info.type == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
        {
            setTexture(binding_info.bind_point, nullptr);
        }
    }

    addDependency(shader, true);
    addDependency(render_pass, true);
    addDependency(pipeline, false);
}
