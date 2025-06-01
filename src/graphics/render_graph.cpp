#include "render_graph.h"

#include "material.h"
#include "render_pass.h"
#include "image.h"
#include "swapchain.h"
#include "resource_manager.h"

const VkImageUsageFlags IMAGE_USAGE = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
const VkFormat EXTRA_FORMAT = VK_FORMAT_R16G16B16A16_SNORM;
const VkFormat DEPTH_FORMAT = VK_FORMAT_D32_SFLOAT;

using namespace std;

PTRGGraph::PTRGGraph(VkDevice _device, PTSwapchain* _swapchain)
{
	device = _device;
	swapchain = _swapchain;
	addDependency(swapchain);

	array<VkDescriptorPoolSize, 2> pool_sizes{ };
	pool_sizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	pool_sizes[0].descriptorCount = MAX_FRAMES_IN_FLIGHT * 256 * 16;
	pool_sizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	pool_sizes[1].descriptorCount = MAX_FRAMES_IN_FLIGHT * 256 * 16;

	VkDescriptorPoolCreateInfo pool_create_info{ };
	pool_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	pool_create_info.maxSets = MAX_FRAMES_IN_FLIGHT * 256;
	pool_create_info.poolSizeCount = static_cast<uint32_t>(pool_sizes.size());
	pool_create_info.pPoolSizes = pool_sizes.data();
	pool_create_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

	if (vkCreateDescriptorPool(device, &pool_create_info, nullptr, &descriptor_pool) != VK_SUCCESS)
		throw runtime_error("unable to create descriptor pool");

	generateRenderPasses();
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

	// create shared scene and transform buffers used by all steps
	shared_transform_uniforms = PTResourceManager::get()->createBuffer(sizeof(TransformUniforms), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	addDependency(shared_transform_uniforms, false);
	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		shared_scene_uniforms[i] = PTResourceManager::get()->createBuffer(sizeof(SceneUniforms), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		addDependency(shared_scene_uniforms[i], false);
	}
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
			spare_depth_image = PTResourceManager::get()->createImage(swapchain->getExtent(), DEPTH_FORMAT, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
			spare_depth_image_view = spare_depth_image->createImageView(VK_IMAGE_ASPECT_DEPTH_BIT);
		}
		else if (step.depth_buffer_binding >= image_buffers.size())
		{
			PTImage* new_image = PTResourceManager::get()->createImage(swapchain->getExtent(), DEPTH_FORMAT, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
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
	// FIXME: ensure that materials are unique to each step
	for (PTRGStep& step : timeline_steps)
	{
		if (step.is_camera_step)
			continue;

		for (const auto& pair : step.process_inputs)
			step.process_material->setTexture(pair.second, image_buffers[pair.first].first, VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_FILTER_NEAREST, (image_buffers[pair.first].first->getFormat() != DEPTH_FORMAT) ? VK_IMAGE_ASPECT_COLOR_BIT : VK_IMAGE_ASPECT_DEPTH_BIT);

		std::array<VkDescriptorSetLayout, MAX_FRAMES_IN_FLIGHT> layouts;
		layouts.fill(step.process_material->getShader()->getDescriptorSetLayout());
		VkDescriptorSetAllocateInfo set_allocation_info{ };
		set_allocation_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		set_allocation_info.descriptorPool = descriptor_pool;
		set_allocation_info.descriptorSetCount = static_cast<uint32_t>(layouts.size());
		set_allocation_info.pSetLayouts = layouts.data();

		if (vkAllocateDescriptorSets(device, &set_allocation_info, step.descriptor_sets.data()) != VK_SUCCESS)
			throw runtime_error("unable to allocate descriptor sets");

		// hook the descriptor sets up to the transform, scene, and material uniform buffers
		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			VkDescriptorBufferInfo transform_buffer_info{ };
			transform_buffer_info.buffer = shared_transform_uniforms->getBuffer();
			transform_buffer_info.offset = 0;
			transform_buffer_info.range = sizeof(TransformUniforms);

			VkDescriptorBufferInfo scene_buffer_info{ };
			scene_buffer_info.buffer = shared_scene_uniforms[i]->getBuffer();
			scene_buffer_info.offset = 0;
			scene_buffer_info.range = sizeof(SceneUniforms);

			VkWriteDescriptorSet write_set{ };
			write_set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			write_set.dstSet = step.descriptor_sets[i];
			write_set.dstBinding = TRANSFORM_UNIFORM_BINDING;
			write_set.dstArrayElement = 0;
			write_set.descriptorCount = 1;
			write_set.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			write_set.pBufferInfo = &transform_buffer_info;

			VkWriteDescriptorSet write_set2{ };
			write_set2.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			write_set2.dstSet = step.descriptor_sets[i];
			write_set2.dstBinding = SCENE_UNIFORM_BINDING;
			write_set2.dstArrayElement = 0;
			write_set2.descriptorCount = 1;
			write_set2.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			write_set2.pBufferInfo = &scene_buffer_info;

			std::array<VkWriteDescriptorSet, 2> write_sets = { write_set, write_set2 };

			vkUpdateDescriptorSets(device, static_cast<uint32_t>(write_sets.size()), write_sets.data(), 0, nullptr);
			step.process_material->applySetWrites(step.descriptor_sets[i]);
		}
	}
}

void PTRGGraph::discardAllResources()
{
	removeDependency(shared_transform_uniforms);
	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		removeDependency(shared_scene_uniforms[i]);

	destroyImages();

	removeDependency(render_pass);

	vkDestroyDescriptorPool(device, descriptor_pool, nullptr);
	removeDependency(swapchain);
}

void PTRGGraph::destroyImages()
{
	for (VkFramebuffer fb : framebuffers)
		vkDestroyFramebuffer(device, fb, nullptr);
	framebuffers.clear();

	for (auto pair : image_buffers)
	{
		vkDestroyImageView(device, pair.second, nullptr);
		removeDependency(pair.first);
	}
	image_buffers.clear();

	if (spare_colour_image != nullptr)
	{
		vkDestroyImageView(device, spare_colour_image_view, nullptr);
		removeDependency(spare_colour_image);
		spare_colour_image = nullptr;
	}
	if (spare_depth_image != nullptr)
	{
		vkDestroyImageView(device, spare_depth_image_view, nullptr);
		removeDependency(spare_depth_image);
		spare_depth_image = nullptr;
	}
	if (spare_normal_image != nullptr)
	{
		vkDestroyImageView(device, spare_normal_image_view, nullptr);
		removeDependency(spare_normal_image);
		spare_normal_image = nullptr;
	}
	if (spare_extra_image != nullptr)
	{
		vkDestroyImageView(device, spare_extra_image_view, nullptr);
		removeDependency(spare_extra_image);
		spare_extra_image = nullptr;
	}
}

PTRGStepInfo PTRGGraph::getStepInfo(size_t step_index) const
{
	PTRGStepInfo step_info{ };
	step_info.render_pass = render_pass;
	step_info.framebuffer = framebuffers[step_index];
	step_info.extent = swapchain->getExtent();
	int col_binding = timeline_steps[step_index].colour_buffer_binding;
	step_info.colour_is_needed = col_binding >= 0;
	step_info.colour_image = step_info.colour_is_needed ? image_buffers[col_binding].first : spare_colour_image;
	int dep_binding = timeline_steps[step_index].depth_buffer_binding;
	step_info.depth_is_needed = dep_binding >= 0;
	step_info.depth_image = step_info.depth_is_needed ? image_buffers[dep_binding].first : spare_depth_image;
	int nor_binding = timeline_steps[step_index].normal_buffer_binding;
	step_info.normal_is_needed = nor_binding >= 0;
	step_info.normal_image = step_info.normal_is_needed ? image_buffers[nor_binding].first : spare_normal_image;
	int ext_binding = timeline_steps[step_index].extra_buffer_binding;
	step_info.extra_is_needed = ext_binding >= 0;
	step_info.extra_image = step_info.extra_is_needed ? image_buffers[ext_binding].first : spare_extra_image;
	step_info.clear_values[0].color = { { 1.0f, 0.0f, 1.0f, 1.0f } };
	step_info.clear_values[1].color = { { 0.0f, 0.0f, 0.0f, 1.0f } };
	step_info.clear_values[2].color = { { 0.0f, 0.0f, 0.0f, 1.0f } };
	step_info.clear_values[3].depthStencil = { 1.0f, 0 };

	return step_info;
}

void PTRGGraph::resize()
{
	destroyImages();
	generateImagesAndFramebuffers();

	for (size_t i = 0; i < timeline_steps.size(); i++)
	{
		PTRGStep step = timeline_steps[i];

		if (step.is_camera_step)
			continue;

		for (const auto& pair : step.process_inputs)
			step.process_material->setTexture(pair.second, image_buffers[pair.first].first);

		for (size_t j = 0; j < MAX_FRAMES_IN_FLIGHT; j++)
			step.process_material->applySetWrites(step.descriptor_sets[j]);
	}
}

void PTRGGraph::updateUniforms(const SceneUniforms& scene_uniforms, const TransformUniforms& transform_uniforms, uint32_t frame_index)
{
	memcpy(shared_scene_uniforms[frame_index]->map(), &scene_uniforms, sizeof(scene_uniforms));
	memcpy(shared_transform_uniforms->map(), &transform_uniforms, sizeof(transform_uniforms));
}

void PTRGGraph::configure(const std::vector<PTRGStep>& steps, int final_image)
{
	final_image_index = final_image;
	destroyImages();

	timeline_steps.clear();

	for (const PTRGStep& step : steps)
		timeline_steps.push_back(step);

	generateImagesAndFramebuffers();
	createMaterialDescriptorSets();
}
