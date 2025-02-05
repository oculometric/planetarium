#pragma once

#include <vulkan/vulkan.h>
#include <vector>

#include "resource.h"
#include "shader.h"
#include "render_pass.h"
#include "swapchain.h"

class PTPipeline : public PTResource
{
    friend class PTResourceManager;
private:
    VkDevice device = VK_NULL_HANDLE;

    VkPipeline pipeline = VK_NULL_HANDLE;
    VkPipelineLayout layout = VK_NULL_HANDLE;
    std::vector<VkDynamicState> dynamic_states;

    PTShader* shader = nullptr;
    PTRenderPass* render_pass = nullptr;

    VkViewport viewport;
    VkRect2D scissor;

    // TODO: setters for these, which recreate the pipeline resources
    VkBool32 depth_write;
    VkBool32 depth_test;
    VkCompareOp depth_op;

    VkCullModeFlags culling;
    VkFrontFace winding_order;

    VkPolygonMode polygon_mode;

    PTPipeline(VkDevice _device, PTShader* _shader, PTRenderPass* _render_pass, PTSwapchain* _swapchain, VkBool32 _depth_write, VkBool32 _depth_test, VkCompareOp _depth_op, VkCullModeFlags _culling, VkFrontFace _winding_order, VkPolygonMode _polygon_mode, std::vector<VkDynamicState> _dynamic_states);

    ~PTPipeline();

public:
    PTPipeline() = delete;
    PTPipeline(const PTPipeline& other) = delete;
    PTPipeline(const PTPipeline&& other) = delete;
    PTPipeline operator=(const PTPipeline& other) = delete;
    PTPipeline operator=(const PTPipeline&& other) = delete;

    inline VkPipeline getPipeline() const { return pipeline; }
    inline VkPipelineLayout getLayout() const { return layout; }
    inline std::vector<VkDynamicState> getAllDynamicStates() const { return dynamic_states; }
    inline VkDynamicState getDynamicState(uint32_t index) const { return dynamic_states[index]; }
    inline PTShader* getShader() const { return shader; }
    inline PTRenderPass* getRenderPass() const { return render_pass; }
    inline VkViewport getViewport() const { return viewport; }
    inline VkRect2D getScissor() const { return scissor; }
    inline VkBool32 getDepthWriteEnabled() const { return depth_write; }
    inline VkBool32 getDepthTestEnabled() const { return depth_test; }
    inline VkCompareOp getDepthCompareOp() const { return depth_op; }
    inline VkCullModeFlags getCullingMode() const { return culling; }
    inline VkFrontFace getWindingOrder() const { return winding_order; }
    inline VkPolygonMode getPolygonMode() const { return polygon_mode; }

};