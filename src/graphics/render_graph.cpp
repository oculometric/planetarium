#include "render_graph.h"

#include <set>

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
	addDependency(swapchain);

	// construct descriptor pool for internal use
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

	// create render pass and material uniform buffers
	generateRenderPassAndUniformBuffers();
}

PTRGGraph::~PTRGGraph()
{
	discardAllResources();
}

void PTRGGraph::generateRenderPassAndUniformBuffers()
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

	// create shared scene and transform uniform buffers used by all steps
	shared_transform_uniforms = PTResourceManager::get()->createBuffer(sizeof(TransformUniforms), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	addDependency(shared_transform_uniforms, false);
	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		shared_scene_uniforms[i] = PTResourceManager::get()->createBuffer(sizeof(SceneUniforms), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		addDependency(shared_scene_uniforms[i], false);
	}
}

VkImageView PTRGGraph::prepareImage(PTImage*& target, VkFormat format, VkExtent2D extent)
{
	VkImageUsageFlags usage = IMAGE_USAGE;
	if (format == DEPTH_FORMAT)
		usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

	target = PTResourceManager::get()->createImage(extent, format, VK_IMAGE_TILING_OPTIMAL, usage, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	return target->createImageView((format == DEPTH_FORMAT) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT);
}

void PTRGGraph::createImageBufferForBinding(int& binding, PTImage*& spare_image, VkImageView& spare_image_view, VkFormat format, VkExtent2D _extent)
{
	VkExtent2D extent = _extent;
	if (extent.width == 0 || extent.height == 0)
		extent = swapchain->getExtent();
	// prepare colour buffer
	if (binding < 0)
	{
		// initialise spare colour buffer
		if (spare_image == nullptr)
			spare_image_view = prepareImage(spare_image, format, extent);
		else if (extent.width != spare_image->getSize().width || extent.height != spare_image->getSize().height)
		{
			debugLog("ERROR: render graph spare image re-used in an incorrect size. this will generate vulkan errors");
		}
	}
	else
	{
		// initialise a new colour buffer in the image buffer array or check usage
		if (binding >= image_buffers.size())
		{
			PTImage* tmp;
			image_buffers.push_back({ tmp, prepareImage(tmp, format, extent) });
		}
		else if (image_buffers[binding].first->getFormat() != format)
		{
			// if texture is being reused by the wrong attachment, just replace this binding with the spare image
			debugLog("ERROR: render graph image re-used in incorrect format. discarding second usage");
			binding = -1;
			if (spare_image == nullptr)
				spare_image_view = prepareImage(spare_image, format, extent);
			else if (extent.width != spare_image->getSize().width || extent.height != spare_image->getSize().height)
			{
				debugLog("ERROR: render graph spare image re-used in an incorrect size. this will generate vulkan errors");
			}
		}
		else if (image_buffers[binding].first->getSize().width != extent.width || image_buffers[binding].first->getSize().height != extent.height)
		{
			debugLog("ERROR: render graph image re-used in an incorrect size. discarding second usage");
			binding = -1;
			if (spare_image == nullptr)
				spare_image_view = prepareImage(spare_image, format, extent);
			else if (extent.width != spare_image->getSize().width || extent.height != spare_image->getSize().height)
			{
				debugLog("ERROR: render graph spare image re-used in an incorrect size. this will generate vulkan errors");
			}
		}
	}
}

void PTRGGraph::generateImagesAndFramebuffers()
{
	for (PTRGStep& step : timeline_steps)
	{
		// prepare colour buffer
		createImageBufferForBinding(step.colour_buffer_binding, spare_colour_image, spare_colour_image_view, swapchain->getImageFormat(), step.custom_extent);

		// prepare depth buffer
		createImageBufferForBinding(step.depth_buffer_binding, spare_depth_image, spare_depth_image_view, DEPTH_FORMAT, step.custom_extent);

		// prepare normal buffer
		createImageBufferForBinding(step.normal_buffer_binding, spare_normal_image, spare_normal_image_view, EXTRA_FORMAT, step.custom_extent);

		// prepare extra buffer
		createImageBufferForBinding(step.extra_buffer_binding, spare_extra_image, spare_extra_image_view, EXTRA_FORMAT, step.custom_extent);

		// create framebuffer using render pass and images
		VkFramebufferCreateInfo framebuffer_create_info{ };
		framebuffer_create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebuffer_create_info.renderPass = render_pass->getRenderPass();
		framebuffer_create_info.attachmentCount = 4;
		VkExtent2D extent = step.custom_extent;
		if (extent.width == 0 || extent.height == 0)
			extent = swapchain->getExtent();
		framebuffer_create_info.width = extent.width;
		framebuffer_create_info.height = extent.height;
		framebuffer_create_info.layers = 1;
		VkImageView attachments[] = 
		{
			(step.colour_buffer_binding == -1) ? spare_colour_image_view : image_buffers[step.colour_buffer_binding].second,
			(step.normal_buffer_binding == -1) ? spare_normal_image_view : image_buffers[step.normal_buffer_binding].second,
			(step.extra_buffer_binding == -1) ? spare_extra_image_view : image_buffers[step.extra_buffer_binding].second,
			(step.depth_buffer_binding == -1) ? spare_depth_image_view : image_buffers[step.depth_buffer_binding].second
		};
		framebuffer_create_info.pAttachments = attachments;

		if (vkCreateFramebuffer(device, &framebuffer_create_info, nullptr, &step.framebuffer) != VK_SUCCESS)
			throw runtime_error("unable to create framebuffer");
	}

	// ensure all of the images are dependencies
	if (spare_colour_image != nullptr) addDependency(spare_colour_image, false);
	if (spare_depth_image != nullptr) addDependency(spare_depth_image, false);
	if (spare_normal_image != nullptr) addDependency(spare_normal_image, false);
	if (spare_extra_image != nullptr) addDependency(spare_extra_image, false);

	for (const auto& pair : image_buffers) addDependency(pair.first, false);
}

void PTRGGraph::linkTexturesToMaterial(const PTRGStep& step)
{
	for (const auto& pair : step.process_inputs)
	{
		if (pair.first == step.colour_buffer_binding || pair.first == step.depth_buffer_binding || pair.first == step.normal_buffer_binding || pair.first == step.extra_buffer_binding)
		{
			debugLog("ERROR: render graph process step is using a texture as both an input and a render attachment, which is not allowed. it will be unbound");
			continue;
		}
		if (pair.first >= image_buffers.size())
		{
			debugLog("ERROR: render graph process step is referencing a texture which does not exist. it will be unbound");
			continue;
		}
		if (pair.first < 0)
		{
			debugLog("WARNING: render graph process step is bound to spare image buffers, which is not allowed. it will be unbound");
			continue;
		}
		step.process_material->setTexture(pair.second, image_buffers[pair.first].first, VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_FILTER_NEAREST, (image_buffers[pair.first].first->getFormat() != DEPTH_FORMAT) ? VK_IMAGE_ASPECT_COLOR_BIT : VK_IMAGE_ASPECT_DEPTH_BIT);
	}
}

void PTRGGraph::createMaterialDescriptorSets()
{
	std::set<PTMaterial*> materials_set;
	for (PTRGStep& step : timeline_steps)
	{
		if (step.is_camera_step)
			continue;

		if (materials_set.contains(step.process_material))
			debugLog("WARNING: material asset used in multiple render graph steps. this will cause undefined behaviour for all but the last instance");
		materials_set.insert(step.process_material);

		// link material texture slots to image buffers
		linkTexturesToMaterial(step);

		// allocate descriptor sets for the material
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
	// release uniform buffers
	removeDependency(shared_transform_uniforms);
	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		removeDependency(shared_scene_uniforms[i]);

	// destroy images and views
	destroyImages();

	// kill render pass
	removeDependency(render_pass);

	// destroy descriptors and pool
	vkDestroyDescriptorPool(device, descriptor_pool, nullptr);
	removeDependency(swapchain);
}

void PTRGGraph::destroyImages()
{
	// destroy framebuffers first
	for (PTRGStep& step : timeline_steps)
	{
		vkDestroyFramebuffer(device, step.framebuffer, nullptr);
		step.framebuffer = VK_NULL_HANDLE;
	}

	// destroy image views and images in the array
	for (auto pair : image_buffers)
	{
		vkDestroyImageView(device, pair.second, nullptr);
		removeDependency(pair.first);
	}
	image_buffers.clear();

	// destroy spare image views and images
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
	PTRGStep step = timeline_steps[step_index];

	PTRGStepInfo step_info{ };
	// assign info necessary for starting a render pass
	step_info.render_pass = render_pass;
	step_info.framebuffer = step.framebuffer;
	step_info.extent = (step.custom_extent.width == 0 || step.custom_extent.height == 0) ? swapchain->getExtent() : step.custom_extent;
	// assign clear values for each attachment
	PTVector4f c_col = step.colour_clear_value;
	PTVector4f c_nor = step.normal_clear_value;
	PTVector4f c_ext = step.extra_clear_value;
	float c_dep = step.depth_clear_value;
	step_info.clear_values[0].color = { { c_col.x, c_col.y, c_col.z, c_col.w } };
	step_info.clear_values[1].color = { { c_nor.x, c_nor.y, c_nor.z, c_nor.w } };
	step_info.clear_values[2].color = { { c_ext.x, c_ext.y, c_ext.z, c_ext.w } };
	step_info.clear_values[3].depthStencil = { c_dep, 0 };

	return step_info;
}

void PTRGGraph::resize()
{
	// destroy the images and regenerate them
	destroyImages();
	generateImagesAndFramebuffers();

	// re-hook up all the textures to the materials
	for (size_t i = 0; i < timeline_steps.size(); i++)
	{
		PTRGStep step = timeline_steps[i];

		if (step.is_camera_step)
			continue;

		linkTexturesToMaterial(step);

		for (size_t j = 0; j < MAX_FRAMES_IN_FLIGHT; j++)
			step.process_material->applySetWrites(step.descriptor_sets[j]);
	}
}

void PTRGGraph::updateUniforms(const SceneUniforms& scene_uniforms, const TransformUniforms& transform_uniforms, uint32_t frame_index)
{
	// update the shared buffers holding uniforms
	memcpy(shared_scene_uniforms[frame_index]->map(), &scene_uniforms, sizeof(scene_uniforms));
	memcpy(shared_transform_uniforms->map(), &transform_uniforms, sizeof(transform_uniforms));
}

void PTRGGraph::configure(const std::vector<PTRGStep>& steps, int final_image)
{
	// destroy images
	destroyImages();

	// copy configuration into internal array
	final_image_index = final_image;
	timeline_steps.clear();
	for (const PTRGStep& step : steps)
		timeline_steps.push_back(step);

	// generate images and descriptors
	generateImagesAndFramebuffers();
	createMaterialDescriptorSets();

	if (final_image < -1 || final_image >= image_buffers.size())
	{
		final_image = -1;
		createImageBufferForBinding(final_image, spare_colour_image, spare_colour_image_view, swapchain->getImageFormat(), swapchain->getExtent());
	}
}
