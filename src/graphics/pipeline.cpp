#include "pipeline.h"

#include "mesh.h"
#include "shader.h"
#include "render_pass.h"
#include "swapchain.h"
#include "render_server.h"

using namespace std;

PTPipeline_T::PTPipeline_T(PTShader _shader, PTRenderPass _render_pass, PTSwapchain _swapchain, VkBool32 _depth_write, VkBool32 _depth_test, VkCompareOp _depth_op, VkCullModeFlags _culling, VkFrontFace _winding_order, VkPolygonMode _polygon_mode, vector<VkDynamicState> _dynamic_states)
{
    device = PTRenderServer::get()->getDevice();
    
    shader = _shader;
    render_pass = _render_pass;

    // we now depend on the shader and the render pass
    
    depth_write = _depth_write;
    depth_test = _depth_test;
    depth_op = _depth_op;

    culling = _culling;
    winding_order = _winding_order;
    polygon_mode = _polygon_mode;

    dynamic_states = _dynamic_states;
    // additional dynamic states, so we don't need to recreate the pipeline every fucking time
    // relies on these being set during command buffer recording
    dynamic_states.push_back(VK_DYNAMIC_STATE_VIEWPORT);
    dynamic_states.push_back(VK_DYNAMIC_STATE_SCISSOR);

    // we create these stupid dummy viewport and scissor as well. dumb stupid viewport
    viewport = VkViewport{ };
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)_swapchain->getExtent().width;
    viewport.height = (float)_swapchain->getExtent().height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    // i actually had a dream about this a while ago, and weirdly i'm getting deja vu writing this comment
    scissor = VkRect2D{ };
    scissor.offset = { 0, 0 };
    scissor.extent = _swapchain->getExtent();

    // create the pipeline layout
    VkDescriptorSetLayout descriptor_set_layout = shader->getDescriptorSetLayout();
    VkPipelineLayoutCreateInfo pipeline_layout_create_info{ };
    pipeline_layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_create_info.setLayoutCount = 1;
    pipeline_layout_create_info.pSetLayouts = &descriptor_set_layout;
    pipeline_layout_create_info.pushConstantRangeCount = 0;
    pipeline_layout_create_info.pPushConstantRanges = nullptr;

    if (vkCreatePipelineLayout(device, &pipeline_layout_create_info, nullptr, &layout) != VK_SUCCESS)
        throw runtime_error("unable to create pipeline layout");

    preparePipeline();
}

void PTPipeline_T::preparePipeline()
{
    // info about the dynamic states
    VkPipelineDynamicStateCreateInfo dynamic_state_create_info{ };
    dynamic_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamic_state_create_info.dynamicStateCount = static_cast<uint32_t>(dynamic_states.size());
    dynamic_state_create_info.pDynamicStates = dynamic_states.data();

    // info about binding the vertex buffer
    auto binding_description = PTMesh_T::getVertexBindingDescription();
    auto attribute_descriptions = PTMesh_T::getVertexAttributeDescriptions();

    VkPipelineVertexInputStateCreateInfo vertex_input_create_info{ };
    vertex_input_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertex_input_create_info.vertexBindingDescriptionCount = 1;
    vertex_input_create_info.pVertexBindingDescriptions = &binding_description;
    vertex_input_create_info.vertexAttributeDescriptionCount = static_cast<uint32_t>(attribute_descriptions.size());
    vertex_input_create_info.pVertexAttributeDescriptions = attribute_descriptions.data();

    // info about the depth stencil state
    VkPipelineDepthStencilStateCreateInfo depth_create_info{ };
    depth_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depth_create_info.depthWriteEnable = depth_write;
    depth_create_info.depthTestEnable = depth_test;
    depth_create_info.depthCompareOp = depth_op;
    depth_create_info.depthBoundsTestEnable = VK_FALSE;
    depth_create_info.stencilTestEnable = VK_FALSE;

    // info about the shape of the index buffer
    // TODO: needs options to switch to line mode
    VkPipelineInputAssemblyStateCreateInfo input_assembly_create_info{ };
    input_assembly_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_assembly_create_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    input_assembly_create_info.primitiveRestartEnable = VK_FALSE;

    // info about the viewport and scissor. the dumb stupid viewport
    VkPipelineViewportStateCreateInfo viewport_state_create_info{ };
    viewport_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_state_create_info.viewportCount = 1;
    viewport_state_create_info.pViewports = &viewport;
    viewport_state_create_info.scissorCount = 1;
    viewport_state_create_info.pScissors = &scissor;
    
    // info about the rasteriser state
    VkPipelineRasterizationStateCreateInfo rasteriser_create_info{ };
    rasteriser_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasteriser_create_info.depthClampEnable = VK_FALSE;
    rasteriser_create_info.rasterizerDiscardEnable = VK_FALSE;
    rasteriser_create_info.polygonMode = polygon_mode;
    rasteriser_create_info.lineWidth = 1.0f;
    rasteriser_create_info.cullMode = culling;
    rasteriser_create_info.frontFace = winding_order;
    rasteriser_create_info.depthBiasEnable = VK_FALSE;
    rasteriser_create_info.depthBiasConstantFactor = 0.0f;
    rasteriser_create_info.depthBiasClamp = 0.0f;
    rasteriser_create_info.depthBiasSlopeFactor = 0.0f;

    // info about sampling. we don't care basically
    VkPipelineMultisampleStateCreateInfo multisampling_create_info{ };
    multisampling_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling_create_info.sampleShadingEnable = VK_FALSE;
    multisampling_create_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampling_create_info.minSampleShading = 1.0f;
    multisampling_create_info.pSampleMask = nullptr;
    multisampling_create_info.alphaToCoverageEnable = VK_FALSE;
    multisampling_create_info.alphaToOneEnable = VK_FALSE;

    // grab shader stage info from the shader (probably should ever be two, vert and frag)
    vector<VkPipelineShaderStageCreateInfo> shader_stages = shader->getStageCreateInfo();

    // grab the colour attachments and copy out their blend state info
    auto attachments = render_pass->getAttachments();
    vector<VkPipelineColorBlendAttachmentState> colour_blend_attachments;
    for (const auto& attachment : attachments)
        colour_blend_attachments.push_back(attachment.blend_state);

    VkPipelineColorBlendStateCreateInfo colour_blend_create_info{ };
    colour_blend_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colour_blend_create_info.logicOpEnable = VK_FALSE;
    colour_blend_create_info.logicOp = VK_LOGIC_OP_COPY;
    colour_blend_create_info.attachmentCount = static_cast<uint32_t>(colour_blend_attachments.size());
    colour_blend_create_info.pAttachments = colour_blend_attachments.data();
    colour_blend_create_info.blendConstants[0] = 0.0f;    // i'm sure having these all be zero is fine
    colour_blend_create_info.blendConstants[1] = 0.0f;
    colour_blend_create_info.blendConstants[2] = 0.0f;
    colour_blend_create_info.blendConstants[3] = 0.0f;

    // tie it all together!
    VkGraphicsPipelineCreateInfo pipeline_create_info{ };
    pipeline_create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline_create_info.stageCount = 2;
    pipeline_create_info.pStages = shader_stages.data();
    pipeline_create_info.pVertexInputState = &vertex_input_create_info;
    pipeline_create_info.pInputAssemblyState = &input_assembly_create_info;
    pipeline_create_info.pViewportState = &viewport_state_create_info;
    pipeline_create_info.pRasterizationState = &rasteriser_create_info;
    pipeline_create_info.pMultisampleState = &multisampling_create_info;
    pipeline_create_info.pColorBlendState = &colour_blend_create_info;
    pipeline_create_info.pDepthStencilState = &depth_create_info;
    pipeline_create_info.pDynamicState = &dynamic_state_create_info;
    pipeline_create_info.layout = layout;
    pipeline_create_info.renderPass = render_pass->getRenderPass();
    pipeline_create_info.subpass = 0;
    pipeline_create_info.basePipelineHandle = VK_NULL_HANDLE;
    pipeline_create_info.basePipelineIndex = -1;

    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipeline_create_info, nullptr, &pipeline) != VK_SUCCESS)
        throw runtime_error("unable to create to create graphics pipeline");
}

PTPipeline_T::~PTPipeline_T()
{
    // destroy the pipeline shit
    vkDestroyPipeline(device, pipeline, nullptr);
    vkDestroyPipelineLayout(device, layout, nullptr);

    // unlink the shader and render pass dependencies
}

inline PTShader PTPipeline_T::getShader() const { return shader; }

inline PTRenderPass PTPipeline_T::getRenderPass() const { return render_pass; }

void PTPipeline_T::setDepthParams(VkBool32 write, VkBool32 test, VkCompareOp op)
{
    // destroy the pipeline (not the layout though)
    vkDestroyPipeline(device, pipeline, nullptr);

    depth_write = write;
    depth_test = test;
    depth_op = op;

    // recreate the pipeline again
    preparePipeline();
}

void PTPipeline_T::setCulling(VkCullModeFlags cull)
{
    // destroy the pipeline (not the layout though)
    vkDestroyPipeline(device, pipeline, nullptr);

    culling = cull;

    // recreate the pipeline again
    preparePipeline();
}

void PTPipeline_T::setPolygonMode(VkPolygonMode mode)
{
    // destroy the pipeline (not the layout though)
    vkDestroyPipeline(device, pipeline, nullptr);

    polygon_mode = mode;

    // recreate the pipeline again
    preparePipeline();
}
