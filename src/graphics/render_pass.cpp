#include "render_pass.h"

#include <stdexcept>

using namespace std;

PTRenderPass::PTRenderPass(VkDevice _device, vector<PTRenderPassAttachment> _attachments, bool enable_depth)
{
    device = _device;
    attachments = _attachments;

    if (attachments.size() == 0)
        throw runtime_error("cannot create render pass with no colour attachment!");

    vector<VkAttachmentDescription> colour_attachments = { };
    vector<VkAttachmentReference> colour_attachment_refs = { };

    uint32_t index = 0;
    for (PTRenderPassAttachment attachment_info : attachments)
    {
        VkAttachmentDescription attachment{ };
        attachment.format = attachment_info.format;
        attachment.samples = VK_SAMPLE_COUNT_1_BIT;
        attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachment.initialLayout = attachment_info.initial_layout;
        attachment.finalLayout = attachment_info.final_layout;

        // TODO: need to be able to configure this according to the needs of the shader (this corresponds to the colour output stuff in the shader, e.g. `layout(location = 0) out vec4 out_colour;`, the location corresponds to the attachment index)
        VkAttachmentReference attachment_ref{ };
        attachment_ref.attachment = index;
        attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        colour_attachments.push_back(attachment);
        colour_attachment_refs.push_back(attachment_ref);

        index++;
    }

    VkAttachmentDescription depth_attachment{ };
    depth_attachment.format = VK_FORMAT_D32_SFLOAT;
    depth_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depth_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depth_attachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depth_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depth_attachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depth_attachment_ref{ };
    depth_attachment_ref.attachment = 1;
    depth_attachment_ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{ };
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = static_cast<uint32_t>(colour_attachment_refs.size());
    subpass.pColorAttachments = colour_attachment_refs.data();
    subpass.pDepthStencilAttachment = &depth_attachment_ref;

    VkSubpassDependency dependency{ };
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    colour_attachments.push_back(depth_attachment);
    VkRenderPassCreateInfo render_pass_create_info{ };
    render_pass_create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_create_info.attachmentCount = static_cast<uint32_t>(colour_attachments.size());
    render_pass_create_info.pAttachments = colour_attachments.data();
    render_pass_create_info.subpassCount = 1;
    render_pass_create_info.pSubpasses = &subpass;
    render_pass_create_info.dependencyCount = 1;
    render_pass_create_info.pDependencies = &dependency;

    if (vkCreateRenderPass(device, &render_pass_create_info, nullptr, &render_pass) != VK_SUCCESS)
        throw runtime_error("unable to create render pass");
}

PTRenderPass::~PTRenderPass()
{
    vkDestroyRenderPass(device, render_pass, nullptr);
}
