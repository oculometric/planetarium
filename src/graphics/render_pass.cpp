#include "render_pass.h"

#include <stdexcept>

using namespace std;

PTRenderPass::PTRenderPass(VkDevice _device, vector<Attachment> _attachments, bool transition_to_readable)
{
    device = _device;
    attachments = _attachments;

    if (attachments.size() == 0)
        throw runtime_error("cannot create render pass with no colour attachment!");

    // track colour attachments and refs
    vector<VkAttachmentDescription> colour_attachments = { };
    vector<VkAttachmentReference> colour_attachment_refs = { };

    uint32_t index = 0;
    for (const Attachment& attachment_info : attachments)
    {
        // add a colour attachment corresponding to the information given
        VkAttachmentDescription attachment{ };
        attachment.format = attachment_info.format;
        attachment.samples = VK_SAMPLE_COUNT_1_BIT;
        attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachment.initialLayout = attachment_info.initial_layout;
        attachment.finalLayout = attachment_info.final_layout;

        VkAttachmentReference attachment_ref{ };
        attachment_ref.attachment = index;        // index refers to the `layout(location = index)` in shader
        attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        colour_attachments.push_back(attachment);
        colour_attachment_refs.push_back(attachment_ref);

        index++;
    }

    // create an additional attachment for the depth buffer
    VkAttachmentDescription depth_attachment{ };
    depth_attachment.format = VK_FORMAT_D32_SFLOAT;
    depth_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depth_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depth_attachment.storeOp = transition_to_readable ? VK_ATTACHMENT_STORE_OP_STORE : VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depth_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depth_attachment.finalLayout = transition_to_readable ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    // here index is always one greater than the last location requested
    VkAttachmentReference depth_attachment_ref{ };
    depth_attachment_ref.attachment = index;
    depth_attachment_ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    colour_attachments.push_back(depth_attachment);

    // create a subpass description, referencing the colour attachments and the depth attachment
    VkSubpassDescription subpass{ };
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = static_cast<uint32_t>(colour_attachment_refs.size());
    subpass.pColorAttachments = colour_attachment_refs.data();
    subpass.pDepthStencilAttachment = &depth_attachment_ref;

    // create dependencies for the subpass
    vector<VkSubpassDependency> dependencies;
    VkSubpassDependency dependency{ };
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.srcAccessMask = VK_ACCESS_NONE_KHR;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    dependency.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
    dependencies.push_back(dependency);

    if (transition_to_readable)
    {
        VkSubpassDependency dependency2{ };
        dependency2.srcSubpass = 0;
        dependency2.dstSubpass = VK_SUBPASS_EXTERNAL;
        dependency2.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependency2.dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        dependency2.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        dependency2.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
        dependency2.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
        dependencies.push_back(dependency2);
    }

    // create a render pass referencing all of the attachments (colour and depth), the subpass and dependency
    VkRenderPassCreateInfo render_pass_create_info{ };
    render_pass_create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_create_info.attachmentCount = static_cast<uint32_t>(colour_attachments.size());
    render_pass_create_info.pAttachments = colour_attachments.data();
    render_pass_create_info.subpassCount = 1;
    render_pass_create_info.pSubpasses = &subpass;
    render_pass_create_info.dependencyCount = static_cast<uint32_t>(dependencies.size());
    render_pass_create_info.pDependencies = dependencies.data();

    if (vkCreateRenderPass(device, &render_pass_create_info, nullptr, &render_pass) != VK_SUCCESS)
        throw runtime_error("unable to create render pass");
}

PTRenderPass::~PTRenderPass()
{
    vkDestroyRenderPass(device, render_pass, nullptr);
}
