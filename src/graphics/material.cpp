#include "material.h"

#include <assert.h>

#include "resource_manager.h"
#include "shader.h"
#include "buffer.h"
#include "pipeline.h"
#include "image.h"
#include "swapchain.h"

using namespace std;

bool checkParamExistsAndType(string name, PTShader::UniformType type, map<string, PTMaterial::MaterialParam> uniforms, PTMaterial::MaterialParam& out)
{
    auto it = uniforms.find(name);
    if (it == uniforms.end())
        return false;
    if (it->second.type != type)
        return false;
    out = it->second;
    return true;
}

vector<VkDescriptorSet> PTMaterial::getDescriptorSets(uint32_t frame_index) const
{
    // TODO: this should get all the descriptor sets for a given frame, when we ahve more than one
    return vector<VkDescriptorSet>({ descriptor_sets[frame_index] });
}

float PTMaterial::getFloatParam(std::string name) const
{
    MaterialParam param;
    if (!checkParamExistsAndType(name, PTShader::UniformType::FLOAT, uniforms, param))
        return 0.0f;
    
    return param.vec_val.x;
}

PTVector2f PTMaterial::getVec2Param(std::string name) const
{
    MaterialParam param;
    if (!checkParamExistsAndType(name, PTShader::UniformType::VEC2, uniforms, param))
        return PTVector2f{ 0, 0 };
    
    return PTVector2f{ param.vec_val.x, param.vec_val.y };
}

PTVector3f PTMaterial::getVec3Param(std::string name) const
{
    MaterialParam param;
    if (!checkParamExistsAndType(name, PTShader::UniformType::VEC3, uniforms, param))
        return PTVector3f{ 0, 0, 0 };
    
    return PTVector3f{ param.vec_val.x, param.vec_val.y, param.vec_val.z };
}

PTVector4f PTMaterial::getVec4Param(std::string name) const
{
    MaterialParam param;
    if (!checkParamExistsAndType(name, PTShader::UniformType::VEC4, uniforms, param))
        return PTVector4f{ 0, 0, 0, 0 };
    
    return param.vec_val;
}

int PTMaterial::getIntParam(std::string name) const
{
    MaterialParam param;
    if (!checkParamExistsAndType(name, PTShader::UniformType::INT, uniforms, param))
        return 0;
    
    return param.int_val;
}

PTMatrix3f PTMaterial::getMat3Param(std::string name) const
{
    MaterialParam param;
    if (!checkParamExistsAndType(name, PTShader::UniformType::MAT3, uniforms, param))
        return PTMatrix3f();
    
    PTMatrix4f m = param.mat_val;
    return PTMatrix3f{ m.x_0, m.y_0, m.z_0,
                       m.x_1, m.y_1, m.z_1,
                       m.x_2, m.y_2, m.z_2 };
}

PTMatrix4f PTMaterial::getMat4Param(std::string name) const
{
    MaterialParam param;
    if (!checkParamExistsAndType(name, PTShader::UniformType::MAT4, uniforms, param))
        return PTMatrix4f();
    
    return param.mat_val;
}

PTImage* PTMaterial::getTextureParam(std::string name) const
{
    MaterialParam param;
    if (!checkParamExistsAndType(name, PTShader::UniformType::TEXTURE, uniforms, param))
        return nullptr;
    
    return param.tex_val;
}

void PTMaterial::setFloatParam(std::string name, float val)
{
    uniforms[name] = MaterialParam(val);
    updateUniformBuffers();
}

void PTMaterial::setVec2Param(std::string name, PTVector2f val)
{
    uniforms[name] = MaterialParam(val);
    updateUniformBuffers();
}

void PTMaterial::setVec3Param(std::string name, PTVector3f val)
{
    uniforms[name] = MaterialParam(val);
    updateUniformBuffers();
}

void PTMaterial::setVec4Param(std::string name, PTVector4f val)
{
    uniforms[name] = MaterialParam(val);
    updateUniformBuffers();
}

void PTMaterial::setIntParam(std::string name, int val)
{
    uniforms[name] = MaterialParam(val);
    updateUniformBuffers();
}

void PTMaterial::setMat3Param(std::string name, PTMatrix3f val)
{
    uniforms[name] = MaterialParam(val);
    updateUniformBuffers();
}

void PTMaterial::setMat4Param(std::string name, PTMatrix4f val)
{
    uniforms[name] = MaterialParam(val);
    updateUniformBuffers();
}

void PTMaterial::setTextureParam(std::string name, PTImage* val)
{
    uniforms[name] = MaterialParam(val);
    updateUniformBuffers();
}

void PTMaterial::updateUniformBuffers() const
{
    // TODO: materials should handle updating their own uniform buffers
    // FIXME: this!
    assert(false);
}

PTMaterial::PTMaterial(VkDevice _device, VkDescriptorPool _descriptor_pool, PTRenderPass* _render_pass, PTSwapchain* swapchain, PTShader* _shader, std::map<std::string, MaterialParam> params, VkBool32 depth_write, VkBool32 depth_test, VkCompareOp depth_op, VkCullModeFlags culling, VkPolygonMode polygon_mode)
{
    device = _device;
    shader = _shader;
    uniforms = params;
    descriptor_pool = _descriptor_pool;
    render_pass = _render_pass;
    pipeline = PTResourceManager::get()->createPipeline(shader, render_pass, swapchain, depth_write, depth_test, depth_op, culling, VK_FRONT_FACE_COUNTER_CLOCKWISE, polygon_mode, { });

    std::array<VkDescriptorSetLayout, MAX_FRAMES_IN_FLIGHT> layouts;
    layouts.fill(getShader()->getDescriptorSetLayout());
    VkDescriptorSetAllocateInfo set_allocation_info{ };
    set_allocation_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    set_allocation_info.descriptorPool = descriptor_pool;
    set_allocation_info.descriptorSetCount = static_cast<uint32_t>(layouts.size());
    set_allocation_info.pSetLayouts = layouts.data();

    if (vkAllocateDescriptorSets(device, &set_allocation_info, descriptor_sets.data()) != VK_SUCCESS)
        throw runtime_error("unable to allocate descriptor sets");

    size_t descriptors = getShader()->getDescriptorCount();
    
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        PTBuffer* buffer = PTResourceManager::get()->createBuffer(getShader()->getDescriptorSetSize(), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        descriptor_buffers[i] = buffer;
        addDependency(buffer, false);
        
        vector<VkWriteDescriptorSet> set_writes;
        for (size_t b = 1; b < descriptors; b++)
        {
            auto binding_info = getShader()->getDescriptorBinding(b);

            VkDescriptorBufferInfo buffer_info{ };
            buffer_info.buffer = buffer->getBuffer();
            buffer_info.offset = binding_info.second.first;
            buffer_info.range = binding_info.second.second;

            VkWriteDescriptorSet write_set{ };
            write_set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            write_set.dstSet = descriptor_sets[i];
            write_set.dstBinding = binding_info.first;
            write_set.dstArrayElement = 0;
            write_set.descriptorCount = 1;
            write_set.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            write_set.pBufferInfo = &buffer_info;
            set_writes.push_back(write_set);
        }

        vkUpdateDescriptorSets(device, static_cast<uint32_t>(set_writes.size()), set_writes.data(), 0, nullptr);
    }

    addDependency(shader, true);
    addDependency(render_pass, true);
    addDependency(pipeline, false);

    updateUniformBuffers();
}

PTMaterial::~PTMaterial()
{
    for (PTBuffer* buf : descriptor_buffers)
        removeDependency(buf);
    vkFreeDescriptorSets(device, descriptor_pool, descriptor_sets.size(), descriptor_sets.data());

    removeDependency(render_pass);
    removeDependency(shader);
    removeDependency(pipeline);
}
