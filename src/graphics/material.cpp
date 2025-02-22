#include "material.h"

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
}

void PTMaterial::setVec2Param(std::string name, PTVector2f val)
{
    uniforms[name] = MaterialParam(val);
}

void PTMaterial::setVec3Param(std::string name, PTVector3f val)
{
    uniforms[name] = MaterialParam(val);
}

void PTMaterial::setVec4Param(std::string name, PTVector4f val)
{
    uniforms[name] = MaterialParam(val);
}

void PTMaterial::setIntParam(std::string name, int val)
{
    uniforms[name] = MaterialParam(val);
}

void PTMaterial::setMat3Param(std::string name, PTMatrix3f val)
{
    uniforms[name] = MaterialParam(val);
}

void PTMaterial::setMat4Param(std::string name, PTMatrix4f val)
{
    uniforms[name] = MaterialParam(val);
}

void PTMaterial::setTextureParam(std::string name, PTImage* val)
{
    uniforms[name] = MaterialParam(val);
}

PTMaterial::PTMaterial(VkDevice _device, VkDescriptorPool _descriptor_pool, PTRenderPass* _render_pass, PTSwapchain* swapchain, PTShader* _shader, std::map<std::string, MaterialParam> params, VkBool32 depth_write, VkBool32 depth_test, VkCompareOp depth_op, VkCullModeFlags culling, VkPolygonMode polygon_mode)
{
    device = _device;
    shader = _shader;
    uniforms = params;
    descriptor_pool = _descriptor_pool;
    render_pass = _render_pass;
    pipeline = PTResourceManager::get()->createPipeline(shader, render_pass, swapchain, depth_write, depth_test, depth_op, culling, VK_FRONT_FACE_COUNTER_CLOCKWISE, polygon_mode, { });

    std::array<VkDescriptorSetLayout, MAX_FRAMES_IN_FLIGHT> layouts(getShader()->getDescriptorSetLayout());
    VkDescriptorSetAllocateInfo set_allocation_info{ };
    set_allocation_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    set_allocation_info.descriptorPool = descriptor_pool;
    set_allocation_info.descriptorSetCount = static_cast<uint32_t>(layouts.size());
    set_allocation_info.pSetLayouts = layouts.data();

    if (vkAllocateDescriptorSets(device, &set_allocation_info, descriptor_sets.data()) != VK_SUCCESS)
        throw runtime_error("unable to allocate descriptor sets");

    // FIXME: the shader should be allowed to have multiple descriptor sets, and we should set them all up (APART FROM THE COMMON ONE). this requires converting the descriptor_buffers and descriptor_sets to arrays, and allocating MAX_FRAMES_IN_FLIGHT sets for each descriptor
    // TODO: shader needs to know the size of its descriptor buffers, and its binding indexes
    VkDeviceSize buffer_size = getShader()->getDescriptorBufferSize();

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        descriptor_buffers[i] = PTResourceManager::get()->createBuffer(buffer_size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        addDependency(descriptor_buffers[i], false);
        
        VkDescriptorBufferInfo buffer_info{ };
        buffer_info.buffer = descriptor_buffers[i]->getBuffer();
        buffer_info.offset = 0;
        buffer_info.range = buffer_size;

        VkWriteDescriptorSet write_set{ };
        write_set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write_set.dstSet = descriptor_sets[i];
        write_set.dstBinding = getShader()->getDescriptorBufferBinding();
        write_set.dstArrayElement = 0;
        write_set.descriptorCount = 1;
        write_set.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        write_set.pBufferInfo = &buffer_info;

        vkUpdateDescriptorSets(device, 1, &write_set, 0, nullptr);
    }

    addDependency(shader, true);
    addDependency(render_pass, true);
    addDependency(pipeline, false);
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
