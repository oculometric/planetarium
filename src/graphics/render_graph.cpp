#include "render_graph.h"

#include "material.h"
#include "render_pass.h"
#include "image.h"
#include "swapchain.h"
#include "resource_manager.h"

using namespace std;

PTRGGraph::PTRGGraph(VkDevice _device, PTSwapchain* _swapchain)
{
	device = _device;
	swapchain = _swapchain;

	PTRGStep basic_step;
	basic_step.is_camera_step = true;
	basic_step.camera_slot = 0;
	basic_step.colour_buffer_binding = 0;

	timeline_steps.push_back(basic_step);

	generateRenderPasses();
	generateImagesAndFramebuffers();
	updateMaterialTextureBindings();
}

PTRGGraph::~PTRGGraph()
{
	discardAllResources();
}

void PTRGGraph::generateRenderPasses()
{
	for (const PTRGStep& step : timeline_steps)
	{
		PTRenderPass::Attachment colour_attachment;
		colour_attachment.format = swapchain->getImageFormat();
		colour_attachment.final_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		PTRenderPass::Attachment normal_and_extra_attachment;
		normal_and_extra_attachment.format = VK_FORMAT_R16G16B16A16_SNORM;
		normal_and_extra_attachment.final_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		// this will result in a colour attachment, two basic data attachments, and a depth attachment
		// it will also generate dependencies ensuring the images are available to shaders as input after rendering to them
		PTRenderPass* rp = PTResourceManager::get()->createRenderPass({ colour_attachment, normal_and_extra_attachment, normal_and_extra_attachment}, true);
		addDependency(rp, false);
		render_passes.push_back(rp);
	}
}

void PTRGGraph::generateImagesAndFramebuffers()
{
	// TODO: here
	// images need to be in the right format (ie we cant reuse a depth image as a colour image)
}
