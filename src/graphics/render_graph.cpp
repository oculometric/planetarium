#include "render_graph.h"

#include "material.h"
#include "render_pass.h"
#include "image.h"
#include "swapchain.h"
#include "resource_manager.h"

const VkImageUsageFlags IMAGE_USAGE = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
const VkFormat EXTRA_FORMAT = VK_FORMAT_R16G16B16A16_SNORM;

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

	// TODO: allocate descriptor pool

	generateRenderPasses();
	generateImagesAndFramebuffers();
	createMaterialDescriptorSets();
}

PTRGGraph::~PTRGGraph()
{
	discardAllResources();
}

void PTRGGraph::generateRenderPasses()
{
	PTRenderPass::Attachment colour_attachment;
	colour_attachment.format = swapchain->getImageFormat();
	colour_attachment.final_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	PTRenderPass::Attachment normal_and_extra_attachment;
	normal_and_extra_attachment.format = EXTRA_FORMAT;
	normal_and_extra_attachment.final_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	// this will result in a colour attachment, two basic data attachments, and a depth attachment
	// it will also generate dependencies ensuring the images are available to shaders as input after rendering to them
	render_pass = PTResourceManager::get()->createRenderPass({ colour_attachment, normal_and_extra_attachment, normal_and_extra_attachment }, true);
	addDependency(render_pass, false);
}

void PTRGGraph::generateImagesAndFramebuffers()
{
	// FIXME: ensure that textures are not re-used for the wrong purposes!
	for (const PTRGStep& step : timeline_steps)
	{
		// prepare colour buffer
		if (step.colour_buffer_binding == -1 && spare_colour_image == nullptr)
		{
			// initialise spare colour buffer
			spare_colour_image = PTResourceManager::get()->createImage(swapchain->getExtent(), swapchain->getImageFormat(), VK_IMAGE_TILING_OPTIMAL, IMAGE_USAGE, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
			spare_colour_image_view = spare_colour_image->createImageView(VK_IMAGE_ASPECT_COLOR_BIT);
		}
		else if (step.colour_buffer_binding >= image_buffers.size())
		{
			// initialise a new colour buffer in the image buffer array
			PTImage* new_image = PTResourceManager::get()->createImage(swapchain->getExtent(), swapchain->getImageFormat(), VK_IMAGE_TILING_OPTIMAL, IMAGE_USAGE, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
			image_buffers.push_back(pair<PTImage*, VkImageView>(new_image, new_image->createImageView(VK_IMAGE_ASPECT_COLOR_BIT)));
		}

		// prepare depth buffer
		if (step.depth_buffer_binding == -1 && spare_depth_image == nullptr)
		{
			spare_depth_image = PTResourceManager::get()->createImage(swapchain->getExtent(), VK_FORMAT_D32_SFLOAT, VK_IMAGE_TILING_OPTIMAL, IMAGE_USAGE, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
			spare_depth_image_view = spare_depth_image->createImageView(VK_IMAGE_ASPECT_COLOR_BIT);
		}
		else if (step.depth_buffer_binding >= image_buffers.size())
		{
			PTImage* new_image = PTResourceManager::get()->createImage(swapchain->getExtent(), VK_FORMAT_D32_SFLOAT, VK_IMAGE_TILING_OPTIMAL, IMAGE_USAGE, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
			image_buffers.push_back(pair<PTImage*, VkImageView>(new_image, new_image->createImageView(VK_IMAGE_ASPECT_DEPTH_BIT)));
		}

		// prepare normal buffer
		if (step.normal_buffer_binding == -1 && spare_normal_image == nullptr)
		{
			spare_normal_image = PTResourceManager::get()->createImage(swapchain->getExtent(), EXTRA_FORMAT, VK_IMAGE_TILING_OPTIMAL, IMAGE_USAGE, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
			spare_normal_image_view = spare_normal_image->createImageView(VK_IMAGE_ASPECT_COLOR_BIT);
		}
		else if (step.normal_buffer_binding >= image_buffers.size())
		{
			PTImage* new_image = PTResourceManager::get()->createImage(swapchain->getExtent(), EXTRA_FORMAT, VK_IMAGE_TILING_OPTIMAL, IMAGE_USAGE, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
			image_buffers.push_back(pair<PTImage*, VkImageView>(new_image, new_image->createImageView(VK_IMAGE_ASPECT_COLOR_BIT)));
		}

		// prepare normal buffer
		if (step.extra_buffer_binding == -1 && spare_extra_image == nullptr)
		{
			spare_extra_image = PTResourceManager::get()->createImage(swapchain->getExtent(), EXTRA_FORMAT, VK_IMAGE_TILING_OPTIMAL, IMAGE_USAGE, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
			spare_extra_image_view = spare_extra_image->createImageView(VK_IMAGE_ASPECT_COLOR_BIT);
		}
		else if (step.extra_buffer_binding >= image_buffers.size())
		{
			PTImage* new_image = PTResourceManager::get()->createImage(swapchain->getExtent(), EXTRA_FORMAT, VK_IMAGE_TILING_OPTIMAL, IMAGE_USAGE, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
			image_buffers.push_back(pair<PTImage*, VkImageView>(new_image, new_image->createImageView(VK_IMAGE_ASPECT_COLOR_BIT)));
		}

		// create framebuffer using render pass and images
		VkFramebufferCreateInfo framebuffer_create_info{ };
		framebuffer_create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebuffer_create_info.renderPass = render_pass->getRenderPass();
		framebuffer_create_info.attachmentCount = 4;
		framebuffer_create_info.width = swapchain->getExtent().width;
		framebuffer_create_info.height = swapchain->getExtent().height;
		framebuffer_create_info.layers = 1;
		VkImageView attachments[] = 
		{
			(step.colour_buffer_binding == -1) ? spare_colour_image_view : image_buffers[step.colour_buffer_binding].second,
			(step.normal_buffer_binding == -1) ? spare_normal_image_view : image_buffers[step.normal_buffer_binding].second,
			(step.extra_buffer_binding == -1) ? spare_extra_image_view : image_buffers[step.extra_buffer_binding].second,
			(step.depth_buffer_binding == -1) ? spare_depth_image_view : image_buffers[step.depth_buffer_binding].second
		};
		framebuffer_create_info.pAttachments = attachments;

		VkFramebuffer fb;
		if (vkCreateFramebuffer(device, &framebuffer_create_info, nullptr, &fb) != VK_SUCCESS)
			throw runtime_error("unable to create framebuffer");

		framebuffers.push_back(fb);
	}

	// ensure all of the images are dependencies
	if (spare_colour_image != nullptr) addDependency(spare_colour_image, false);
	if (spare_depth_image != nullptr) addDependency(spare_depth_image, false);
	if (spare_normal_image != nullptr) addDependency(spare_normal_image, false);
	if (spare_extra_image != nullptr) addDependency(spare_extra_image, false);

	for (const auto& pair : image_buffers) addDependency(pair.first, false);
}

void PTRGGraph::createMaterialDescriptorSets()
{
	// create shared scene and transform buffers used by all steps here
	shared_transform_uniforms = PTResourceManager::get()->createBuffer(sizeof(TransformUniforms), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	addDependency(shared_transform_uniforms, false);
	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		shared_scene_uniforms[i] = PTResourceManager::get()->createBuffer(sizeof(SceneUniforms), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		addDependency(shared_scene_uniforms[i], false);
	}

	// TODO: ensure that materials are unique to each step
	for (PTRGStep& step : timeline_steps)
	{
		if (step.is_camera_step)
			continue;

		for (const auto& pair : step.process_inputs)
			step.process_material->setTexture(pair.second, image_buffers[pair.second].first);

		std::array<VkDescriptorSetLayout, MAX_FRAMES_IN_FLIGHT> layouts;
		layouts.fill(step.process_material->getShader()->getDescriptorSetLayout());
		VkDescriptorSetAllocateInfo set_allocation_info{ };
		set_allocation_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		set_allocation_info.descriptorPool = descriptor_pool;
		set_allocation_info.descriptorSetCount = static_cast<uint32_t>(layouts.size());
		set_allocation_info.pSetLayouts = layouts.data();

		std::array<VkDescriptorSet, MAX_FRAMES_IN_FLIGHT> sets;

		if (vkAllocateDescriptorSets(device, &set_allocation_info, sets.data()) != VK_SUCCESS)
			throw runtime_error("unable to allocate descriptor sets");

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			VkDescriptorBufferInfo transform_buffer_info{ };
			transform_buffer_info.buffer = shared_transform_uniforms->getBuffer();
			transform_buffer_info.offset = 0;
			transform_buffer_info.range = sizeof(TransformUniforms);

			VkDescriptorBufferInfo scene_buffer_info{ };
			transform_buffer_info.buffer = shared_scene_uniforms[i]->getBuffer();
			transform_buffer_info.offset = 0;
			transform_buffer_info.range = sizeof(SceneUniforms);

			VkWriteDescriptorSet write_set{ };
			write_set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			write_set.dstSet = sets[i];
			write_set.dstBinding = TRANSFORM_UNIFORM_BINDING;
			write_set.dstArrayElement = 0;
			write_set.descriptorCount = 1;
			write_set.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			write_set.pBufferInfo = &transform_buffer_info;

			VkWriteDescriptorSet write_set2{ };
			write_set2.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			write_set2.dstSet = sets[i];
			write_set2.dstBinding = SCENE_UNIFORM_BINDING;
			write_set2.dstArrayElement = 0;
			write_set2.descriptorCount = 1;
			write_set2.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			write_set2.pBufferInfo = &scene_buffer_info;

			std::array<VkWriteDescriptorSet, 2> write_sets = { write_set, write_set2 };

			vkUpdateDescriptorSets(device, write_sets.size(), write_sets.data(), 0, nullptr);
			step.process_material->applySetWrites(sets[i]);
		}

		descriptor_sets.push_back(sets);
	}

	// TODO: need to update transforms and scene uniforms along with the rest of the scene
}

PTRGStepInfo PTRGGraph::getStepInfo(size_t step_index) const
{
	// TODO: generate step info struct
}
