#pragma once

#include <vulkan/vulkan.h>
#include <vector>

#include "shader.h"
#include "render_pass.h"

// TODO: move more stuff into the pipeline class. make all of this more object-orientated
class PTPipeline
{
private:
    VkDevice device;

    VkPipeline pipeline;
    VkPipelineLayout layout;
    VkDescriptorSetLayout descriptor_set_layout;
    std::vector<VkDynamicState> dynamic_states;

    PTShader* shader = nullptr;
    PTRenderPass* render_pass = nullptr;

    VkViewport viewport;
    VkRect2D scissor;

    VkBool32 depth_write;
    VkBool32 depth_test;
    VkCompareOp depth_op;

    VkCullModeFlags culling;
    VkFrontFace winding_order;

    VkPolygonMode polygon_mode;

public:
    PTPipeline() = delete;
    PTPipeline(const PTPipeline& other) = delete;
    PTPipeline(const PTPipeline&& other) = delete;
    PTPipeline operator=(const PTPipeline& other) = delete;
    PTPipeline operator=(const PTPipeline&& other) = delete;

    // TODO: add a list of dynamic states to create to this
    PTPipeline(PTShader* _shader, PTRenderPass* _render_pass, VkBool32 _depth_write, VkBool32 _depth_test, VkCompareOp _depth_op, VkCullModeFlags _culling, VkFrontFace _winding_order, VkPolygonMode _polygon_mode, VkDevice _device, PTSwapchain* _swapchain);

    inline VkPipeline getPipeline() { return pipeline; }
    inline VkPipelineLayout getLayout() { return layout; }
    inline VkDescriptorSetLayout getDescriptorSetLayout() { return descriptor_set_layout; }
    inline std::vector<VkDynamicState> getAllDynamicStates() { return dynamic_states; }
    inline VkDynamicState getDynamicState(uint32_t index) { return dynamic_states[index]; }
    inline PTShader* getShader() { return shader; }
    inline PTRenderPass* getRenderPass() { return render_pass; }
    inline VkViewport getViewport() { return viewport; }
    inline VkRect2D getScissor() { return scissor; }
    inline VkBool32 getDepthWriteEnabled() { return depth_write; }
    inline VkBool32 getDepthTestEnabled() { return depth_test; }
    inline VkCompareOp getDepthCompareOp() { return depth_op; }
    inline VkCullModeFlags getCullingMode() { return culling; }
    inline VkFrontFace getWindingOrder() { return winding_order; }
    inline VkPolygonMode getPolygonMode() { return polygon_mode; }

    // TODO: setters for these, which recreate the pipeline resources


    ~PTPipeline();
};