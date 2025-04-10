#include "material.h"

#include <assert.h>
#include <fstream>

#include "resource_manager.h"
#include "shader.h"
#include "buffer.h"
#include "pipeline.h"
#include "image.h"
#include "swapchain.h"
#include "deserialiser.h"

using namespace std;

PTMaterial::PTMaterial(VkDevice _device, std::string material_path, PTRenderPass* _render_pass, PTSwapchain* swapchain)
{
    device = _device;
    origin_path = material_path;
    render_pass = _render_pass;

    ifstream file(material_path, ios::ate);
    if (!file.is_open())
        throw runtime_error("material path does not exist");

    size_t size = file.tellg();
    string text;
    text.resize(size, ' ');
    file.seekg(0);
    file.read(text.data(), size);

    // TODO: deserialise params, shader, and uniform/textures
    PTDeserialiser::MaterialParams params;
    std::string shader_path;
    std::vector<PTDeserialiser::UniformParam> uniforms;
    std::map<uint16_t, PTResource*> textures;
    PTDeserialiser::deserialiseMaterial(text, params, shader_path, uniforms, textures);

    shader = PTResourceManager::get()->createShader(shader_path, false);
    
    initialiseMaterial(swapchain, params.depth_write, params.depth_test, params.depth_op, params.culling, params.polygon_mode);

    // TODO: apply uniforms and textures
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
    for (auto pair : descriptor_buffers)
    {
        removeDependency(pair.second);
    }

    removeDependency(render_pass);
    removeDependency(shader);
    removeDependency(pipeline);
}

void PTMaterial::applySetWrites(VkDescriptorSet descriptor_set)
{
    vector<VkWriteDescriptorSet> set_writes;
    size_t descriptors = getShader()->getDescriptorCount();
    for (size_t b = 0; b < descriptors; b++)
    {
        auto binding_info = getShader()->getDescriptorBinding(b);
        if (binding_info.bind_point == TRANSFORM_UNIFORM_BINDING
		 || binding_info.bind_point == SCENE_UNIFORM_BINDING)
			   continue;

        VkDescriptorBufferInfo buffer_info{ };
        buffer_info.buffer = descriptor_buffers[binding_info.bind_point]->getBuffer();
        buffer_info.offset = 0;
        buffer_info.range = binding_info.size;

        VkWriteDescriptorSet write_set{ };
        write_set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write_set.dstSet = descriptor_set;
        write_set.dstBinding = binding_info.bind_point;
        write_set.dstArrayElement = 0;
        write_set.descriptorCount = 1;
        write_set.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        write_set.pBufferInfo = &buffer_info;
        set_writes.push_back(write_set);
    }

    vkUpdateDescriptorSets(device, static_cast<uint32_t>(set_writes.size()), set_writes.data(), 0, nullptr);
}

void PTMaterial::initialiseMaterial(PTSwapchain* swapchain, VkBool32 depth_write, VkBool32 depth_test, VkCompareOp depth_op, VkCullModeFlags culling, VkPolygonMode polygon_mode)
{
    pipeline = PTResourceManager::get()->createPipeline(shader, render_pass, swapchain, depth_write, depth_test, depth_op, culling, VK_FRONT_FACE_COUNTER_CLOCKWISE, polygon_mode, { });

    size_t descriptors = getShader()->getDescriptorCount();

    for (size_t b = 0; b < descriptors; b++)
    {
        auto binding_info = getShader()->getDescriptorBinding(b);
        if (binding_info.bind_point == TRANSFORM_UNIFORM_BINDING
            || binding_info.bind_point == SCENE_UNIFORM_BINDING)
            continue;

        PTBuffer* buffer = PTResourceManager::get()->createBuffer(binding_info.size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        descriptor_buffers[binding_info.bind_point] = buffer;
        addDependency(buffer, false);
    }

    addDependency(shader, true);
    addDependency(render_pass, true);
    addDependency(pipeline, false);
}
