#include "material.h"

#include <assert.h>

#include "resource_manager.h"
#include "shader.h"
#include "buffer.h"
#include "pipeline.h"
#include "image.h"
#include "swapchain.h"

using namespace std;

VkDescriptorSet PTMaterial::getDescriptorSet(uint32_t frame_index) const
{
    return descriptor_sets[frame_index];
}

PTMaterial::PTMaterial(VkDevice _device, VkDescriptorPool _descriptor_pool, PTRenderPass* _render_pass, PTSwapchain* swapchain, PTShader* _shader, VkBool32 depth_write, VkBool32 depth_test, VkCompareOp depth_op, VkCullModeFlags culling, VkPolygonMode polygon_mode)
{
    device = _device;
    shader = _shader;
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
        vector<VkWriteDescriptorSet> set_writes;
        for (size_t b = 0; b < descriptors; b++)
        {
            auto binding_info = getShader()->getDescriptorBinding(b);
            if (binding_info.bind_point == COMMON_UNIFORM_BINDING)
                continue;

            PTBuffer* buffer = PTResourceManager::get()->createBuffer(binding_info.size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
            descriptor_buffers[i][binding_info.bind_point] = buffer;
            addDependency(buffer, false);

            VkDescriptorBufferInfo buffer_info{ };
            buffer_info.buffer = buffer->getBuffer();
            buffer_info.offset = 0;
            buffer_info.range = binding_info.size;

            VkWriteDescriptorSet write_set{ };
            write_set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            write_set.dstSet = descriptor_sets[i];
            write_set.dstBinding = binding_info.bind_point;
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
}

PTMaterial::~PTMaterial()
{
    for (auto collection : descriptor_buffers)
    {
        for (auto pair : collection)
            removeDependency(pair.second);
    }
    vkFreeDescriptorSets(device, descriptor_pool, descriptor_sets.size(), descriptor_sets.data());

    removeDependency(render_pass);
    removeDependency(shader);
    removeDependency(pipeline);
}
