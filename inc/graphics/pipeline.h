#pragma once

#include <vulkan/vulkan.h>
#include <vector>

#include "reference_counter.h"

typedef PTCountedPointer<class PTRenderPass_T> PTRenderPass;
typedef PTCountedPointer<class PTShader_T> PTShader;
typedef PTCountedPointer<class PTSwapchain_T> PTSwapchain;

class PTPipeline_T
{
private:
    VkDevice device = VK_NULL_HANDLE;

    VkPipeline pipeline = VK_NULL_HANDLE;
    VkPipelineLayout layout = VK_NULL_HANDLE;
    std::vector<VkDynamicState> dynamic_states;

    PTShader shader = nullptr;
    PTRenderPass render_pass = nullptr;

    VkViewport viewport;
    VkRect2D scissor;

    VkBool32 depth_write;
    VkBool32 depth_test;
    VkCompareOp depth_op;

    VkCullModeFlags culling;
    VkFrontFace winding_order;

    VkPolygonMode polygon_mode;

public:
    PTPipeline_T() = delete;
    PTPipeline_T(const PTPipeline_T& other) = delete;
    PTPipeline_T(const PTPipeline_T&& other) = delete;
    PTPipeline_T operator=(const PTPipeline_T& other) = delete;
    PTPipeline_T operator=(const PTPipeline_T&& other) = delete;
    ~PTPipeline_T();

    static inline PTCountedPointer<PTPipeline_T> createPipeline(PTShader shader, PTRenderPass render_pass, PTSwapchain swapchain, VkBool32 depth_write, VkBool32 depth_test, VkCompareOp depth_op, VkCullModeFlags culling, VkFrontFace winding_order, VkPolygonMode polygon_mode, std::vector<VkDynamicState> dynamic_states)
    { return PTCountedPointer<PTPipeline_T>(new PTPipeline_T(shader, render_pass, swapchain, depth_write, depth_test, depth_op, culling, winding_order, polygon_mode, dynamic_states)); }

    inline VkPipeline getPipeline() const { return pipeline; }
    inline VkPipelineLayout getLayout() const { return layout; }
    inline std::vector<VkDynamicState> getAllDynamicStates() const { return dynamic_states; }
    inline VkDynamicState getDynamicState(uint32_t index) const { return dynamic_states[index]; }
    PTShader getShader() const;
    PTRenderPass getRenderPass() const;
    inline VkViewport getViewport() const { return viewport; }
    inline VkRect2D getScissor() const { return scissor; }
    inline VkBool32 getDepthWriteEnabled() const { return depth_write; }
    inline VkBool32 getDepthTestEnabled() const { return depth_test; }
    inline VkCompareOp getDepthCompareOp() const { return depth_op; }
    inline VkCullModeFlags getCullingMode() const { return culling; }
    inline VkFrontFace getWindingOrder() const { return winding_order; }
    inline VkPolygonMode getPolygonMode() const { return polygon_mode; }

    void setDepthParams(VkBool32 write, VkBool32 test, VkCompareOp op);
    void setCulling(VkCullModeFlags cull);
    void setPolygonMode(VkPolygonMode mode);

private:
    PTPipeline_T(PTShader _shader, PTRenderPass _render_pass, PTSwapchain _swapchain, VkBool32 _depth_write, VkBool32 _depth_test, VkCompareOp _depth_op, VkCullModeFlags _culling, VkFrontFace _winding_order, VkPolygonMode _polygon_mode, std::vector<VkDynamicState> _dynamic_states);

    void preparePipeline();
};

typedef PTCountedPointer<PTPipeline_T> PTPipeline;
