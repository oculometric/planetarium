#pragma once

#include <vulkan/vulkan.h>
#include <vector>

#include "reference_counter.h"

class PTRenderPass_T
{
public:
    struct Attachment
    {
        VkFormat format = VK_FORMAT_B8G8R8A8_SRGB;
        VkImageLayout initial_layout = VK_IMAGE_LAYOUT_UNDEFINED;
        VkImageLayout final_layout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        VkPipelineColorBlendAttachmentState blend_state = VkPipelineColorBlendAttachmentState
        {
            VK_TRUE,
            VK_BLEND_FACTOR_SRC_ALPHA,
            VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
            VK_BLEND_OP_ADD,
            VK_BLEND_FACTOR_ONE,
            VK_BLEND_FACTOR_ZERO,
            VK_BLEND_OP_ADD,
            VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
        };
    };

private:
    VkDevice device = VK_NULL_HANDLE;

    VkRenderPass render_pass = VK_NULL_HANDLE;
    std::vector<Attachment> attachments;

public:
    PTRenderPass_T() = delete;
    PTRenderPass_T(PTRenderPass_T& other) = delete;
    PTRenderPass_T(PTRenderPass_T&& other) = delete;
    PTRenderPass_T operator=(PTRenderPass_T& other) = delete;
    PTRenderPass_T operator=(PTRenderPass_T&& other) = delete;
    ~PTRenderPass_T();

    inline static PTCountedPointer<PTRenderPass_T> createRenderPass(std::vector<Attachment> attachments = { }, bool transition_to_readable = false)
    { return PTCountedPointer<PTRenderPass_T>(new PTRenderPass_T(attachments, transition_to_readable)); }

    inline VkRenderPass getRenderPass() const { return render_pass; }
    inline std::vector<Attachment> getAttachments() const { return attachments; }

private:
    PTRenderPass_T(std::vector<Attachment> _attachments = { }, bool transition_to_readable = false);
};

typedef PTCountedPointer<PTRenderPass_T> PTRenderPass;
