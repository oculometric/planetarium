#include "render_server.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <stdexcept>
#include <cstring>
#include <iostream>
#include <set>
#include <algorithm>
#include <cstdlib>

#include "application.h"
#include "node.h"
#include "shader.h"
#include "pipeline.h"
#include "swapchain.h"
#include "buffer.h"
#include "render_pass.h"
#include "image.h"
#include "mesh.h"
#include "material.h"
#include "debug.h"
#include "resource_manager.h"
#include "bitmap.h"

#define MAX_OBJECTS 512

using namespace std;

#ifndef NDEBUG
static VKAPI_ATTR VkBool32 VKAPI_CALL vulkanDebugMessengerCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
    VkDebugUtilsMessageTypeFlagsEXT message_type,
    const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
    void* user_data)
{
    string severity;
    switch (message_severity)
    {
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT: return VK_FALSE;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT: severity = "INFO"; break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT: severity = "WARNING"; break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT: severity = "ERROR"; break;
        default: return VK_FALSE;
    }

    string type;
    switch(message_type)
    {
        case VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT: type = "GENERAL"; break;
        case VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT: type = "VALIDATION"; break;
        case VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT: type = "PERFORMANCE"; break;
        case VK_DEBUG_UTILS_MESSAGE_TYPE_DEVICE_ADDRESS_BINDING_BIT_EXT: type = "DEVICE ADDRESS BINDING"; break;
        default: return VK_FALSE;
    }

    debugLog("(" + severity + ":" + type + "): " + string(callback_data->pMessage));

    return VK_FALSE;
}
#endif

static PTRenderServer* render_server = nullptr;

void PTRenderServer::init(GLFWwindow* window, vector<const char*> glfw_extensions)
{
	if (render_server != nullptr)
		return;
	
	render_server = new PTRenderServer(window, glfw_extensions);
}

void PTRenderServer::deinit()
{
	if (render_server == nullptr)
		return;

	delete render_server;
	render_server = nullptr;
}

PTRenderServer* PTRenderServer::get()
{
	return render_server;
}

VkCommandBuffer PTRenderServer::beginTransientCommands()
{
    VkCommandBufferAllocateInfo allocation_info{ };
    allocation_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocation_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocation_info.commandPool = command_pool;
    allocation_info.commandBufferCount = 1;

    VkCommandBuffer transient_command_buffer;
    vkAllocateCommandBuffers(device, &allocation_info, &transient_command_buffer);

    VkCommandBufferBeginInfo begin_info{ };
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(transient_command_buffer, &begin_info);

    return transient_command_buffer;
}

void PTRenderServer::endTransientCommands(VkCommandBuffer transient_command_buffer)
{
    vkEndCommandBuffer(transient_command_buffer);

    VkSubmitInfo submit_info{ };
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &transient_command_buffer;

    vkQueueSubmit(queues[PTPhysicalDevice::QueueFamily::GRAPHICS], 1, &submit_info, VK_NULL_HANDLE);
    vkQueueWaitIdle(queues[PTPhysicalDevice::QueueFamily::GRAPHICS]);

    vkFreeCommandBuffers(device, command_pool, 1, &transient_command_buffer);
}

void PTRenderServer::addDrawRequest(PTNode* owner, PTMesh* mesh, PTMaterial* material, PTTransform* target_transform)
{
	if (owner == nullptr || mesh == nullptr)
        return;

    DrawRequest request{ };
    request.mesh = mesh;
    request.material =  (material == nullptr) ? default_material : material;
    request.transform = (target_transform == nullptr) ? owner->getTransform() : target_transform;

    std::array<VkDescriptorSetLayout, MAX_FRAMES_IN_FLIGHT> layouts;
    layouts.fill(request.material->getShader()->getDescriptorSetLayout());
    VkDescriptorSetAllocateInfo set_allocation_info{ };
    set_allocation_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    set_allocation_info.descriptorPool = descriptor_pool;
    set_allocation_info.descriptorSetCount = static_cast<uint32_t>(layouts.size());
    set_allocation_info.pSetLayouts = layouts.data();

    if (vkAllocateDescriptorSets(device, &set_allocation_info, request.descriptor_sets.data()) != VK_SUCCESS)
        throw runtime_error("unable to allocate descriptor sets");

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        request.descriptor_buffers[i] = PTResourceManager::get()->createBuffer(sizeof(TransformUniforms), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        {
            VkDescriptorBufferInfo buffer_info{ };
            buffer_info.buffer = request.descriptor_buffers[i]->getBuffer();
            buffer_info.offset = 0;
            buffer_info.range = sizeof(TransformUniforms);

            VkWriteDescriptorSet write_set{ };
            write_set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            write_set.dstSet = request.descriptor_sets[i];
            write_set.dstBinding = TRANSFORM_UNIFORM_BINDING;
            write_set.dstArrayElement = 0;
            write_set.descriptorCount = 1;
            write_set.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            write_set.pBufferInfo = &buffer_info;

            vkUpdateDescriptorSets(device, 1, &write_set, 0, nullptr);
        }

        {
            VkDescriptorBufferInfo buffer_info{ };
            buffer_info.buffer = scene_uniform_buffers[i]->getBuffer();
            buffer_info.offset = 0;
            buffer_info.range = sizeof(SceneUniforms);

            VkWriteDescriptorSet write_set{ };
            write_set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            write_set.dstSet = request.descriptor_sets[i];
            write_set.dstBinding = SCENE_UNIFORM_BINDING;
            write_set.dstArrayElement = 0;
            write_set.descriptorCount = 1;
            write_set.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            write_set.pBufferInfo = &buffer_info;

            vkUpdateDescriptorSets(device, 1, &write_set, 0, nullptr);
        }

        request.material->applySetWrites(request.descriptor_sets[i]);
    }

    beginEditLock();

    draw_queue.emplace(owner, request);

    endEditLock();
}

void PTRenderServer::removeAllDrawRequests(PTNode* owner)
{
    beginEditLock();

    for (auto[itr, range_end] = draw_queue.equal_range(owner); itr != range_end; ++itr)
    {
        vkFreeDescriptorSets(device, descriptor_pool, static_cast<uint32_t>(itr->second.descriptor_sets.size()), itr->second.descriptor_sets.data());
        for (PTBuffer* buf : itr->second.descriptor_buffers)
            buf->removeReferencer();
    }

    draw_queue.erase(owner);

    endEditLock();
}

void PTRenderServer::beginEditLock()
{
    while (state < 0);

    state++;
}

void PTRenderServer::endEditLock()
{
    if (state <= 0)
        throw runtime_error("cannot end edit lock! there is no edit lock active!");

    state--;
}

void PTRenderServer::beginDrawLock()
{
    while (state != 0);

    state = -1;
}

void PTRenderServer::endDrawLock()
{
    if (state != -1)
        throw runtime_error("cannot end draw lock! there is no draw lock active!");
    
    state = 0;
}

PTRenderServer::PTRenderServer(GLFWwindow* window, vector<const char*> glfw_extensions)
{
	initVulkan(window, glfw_extensions);
}

PTRenderServer::~PTRenderServer()
{
	deinitVulkan();
}

void PTRenderServer::initVulkan(GLFWwindow* window, vector<const char*> glfw_extensions)
{
	debugLog("initialising vulkan...");

    // initialise vulkan app instance
    vector<const char*> layers;
    auto extensions = glfw_extensions;
#ifndef NDEBUG
    extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif
    initVulkanInstance(layers, extensions);

	debugLog("    creating window surface");
    if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS)
        throw runtime_error("unable to create window surface");

    debugLog("    initialising device");
	initDevice(layers);

	PTResourceManager::get()->init(device, physical_device);

	debugLog("    creating swapchain");
	PTVector2u size = PTApplication::get()->getFramebufferSize();
    swapchain = PTResourceManager::get()->createSwapchain(surface, size.x, size.y);
    
	debugLog("    creating render pass");
    PTRenderPass::Attachment colour_att;
    colour_att.format = swapchain->getImageFormat();
    PTRenderPass::Attachment normal_att;
    normal_att.format = VK_FORMAT_R16G16B16A16_SNORM;
    normal_att.final_layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    render_pass = PTResourceManager::get()->createRenderPass({ colour_att, normal_att });

	debugLog("    creating command pool");
	createCommandPoolAndBuffers();

	debugLog("    creating descriptor pool");
	createDescriptorPoolAndSets();

	debugLog("    creating framebuffers");
	createFramebufferAndSyncResources();

	debugLog("    creating default material");
    default_material = PTResourceManager::get()->createMaterial("res/default.ptmat", swapchain, render_pass);

    debugLog("done.");
}

void PTRenderServer::update()
{
    static uint32_t frame_index = 0;
    drawFrame(frame_index);

	if (wants_screenshot)
        takeScreenshot(frame_index);

	frame_index = (frame_index + 1) % MAX_FRAMES_IN_FLIGHT;
}

void PTRenderServer::deinitVulkan()
{
	vkDeviceWaitIdle(device);

    default_material->removeReferencer();

    while (!draw_queue.empty())
        removeAllDrawRequests(draw_queue.begin()->first);

    destroyFramebufferAndSyncResources();

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        scene_uniform_buffers[i]->removeReferencer();
    
    vkDestroyCommandPool(device, command_pool, nullptr);

    render_pass->removeReferencer();

    swapchain->removeReferencer();

    PTResourceManager::deinit();

    vkDestroyDescriptorPool(device, descriptor_pool, nullptr);

    vkDestroyDevice(device, nullptr);

    vkDestroySurfaceKHR(instance, surface, nullptr);

#ifndef NDEBUG
    destroyDebugUtilsMessenger(instance, debug_messenger);
#endif

    vkDestroyInstance(instance, nullptr);
}

void PTRenderServer::initVulkanInstance(vector<const char*>& layers, vector<const char*> extensions)
{
	const char* application_name = "planetarium";
    debugLog("    vulkan app name: " + string(application_name));
    VkApplicationInfo app_info{ };
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName = application_name;
    app_info.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
    app_info.pEngineName = "No Engine";
    app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.apiVersion = VK_API_VERSION_1_3;

#ifndef NDEBUG
    // get debug validation layer
    debugLog("    searching for debug validation layer...");
    uint32_t layer_count;
    vkEnumerateInstanceLayerProperties(&layer_count, nullptr);
    std::vector<VkLayerProperties> available_layers(layer_count);
    vkEnumerateInstanceLayerProperties(&layer_count, available_layers.data());

    bool debug_layer_found = false;
    const char* target_debug_layer = "VK_LAYER_KHRONOS_validation";
    for (VkLayerProperties layer : available_layers)
    {
        if (strcmp(layer.layerName, target_debug_layer))
        {
            debug_layer_found = true;
            break;
        }
    }
    if (!debug_layer_found)
        throw std::runtime_error("unable to access VK debug validation layer");
    debugLog("    debug validation layer found (" + string(target_debug_layer) + ")");
#endif

    // instance creation
    VkInstanceCreateInfo create_info{ };
    create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    create_info.pApplicationInfo = &app_info;
    create_info.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    create_info.ppEnabledExtensionNames = extensions.data();
#ifdef NDEBUG
    create_info.enabledLayerCount = 0;
    create_info.ppEnabledLayerNames = nullptr;
#else
    layers.clear();
    layers.push_back(target_debug_layer);
    create_info.enabledLayerCount = static_cast<uint32_t>(layers.size());
    create_info.ppEnabledLayerNames = layers.data();
#endif
    debugLog("    extensions enabled:");
    for (const char* ext : extensions)
        debugLog("       " + string(ext));

    debugLog("    creating VK instance...");
    VkResult result = vkCreateInstance(&create_info, nullptr, &instance);
    if (result != VK_SUCCESS)
    {
        debugLog("    VK instance creation failed: " + string(/*string_VkResult(result) <<*/));
        throw runtime_error("unable to create VK instance");
    }

#ifndef NDEBUG
    debugLog("    creating debug messenger...");
    VkDebugUtilsMessengerCreateInfoEXT debug_messenger_create_info{ };
    debug_messenger_create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    debug_messenger_create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    debug_messenger_create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    debug_messenger_create_info.pfnUserCallback = vulkanDebugMessengerCallback;
    debug_messenger_create_info.pUserData = nullptr;

    if (createDebugUtilsMessenger(instance, &debug_messenger_create_info, &debug_messenger) != VK_SUCCESS)
        throw runtime_error("unable to set up debug messenger!");
#endif
    debugLog("    done.");
}

void PTRenderServer::initDevice(const std::vector<const char*>& layers)
{
	// find the best physical device
	physical_device = selectPhysicalDevice();

	// identify the queue indices
	vector<VkDeviceQueueCreateInfo> queue_create_infos;
	{
		set<uint32_t> queue_indices;
		float graphics_queue_priority = 1.0f;
		for (pair<PTPhysicalDevice::QueueFamily, uint32_t> queue_family : physical_device.getAllQueueFamilies())
		{
			if (queue_indices.count(queue_family.second) > 0)
				continue;
			VkDeviceQueueCreateInfo queue_create_info{ };
			queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queue_create_info.queueFamilyIndex = queue_family.second;
			queue_create_info.queueCount = 1;
			queue_create_info.pQueuePriorities = &graphics_queue_priority;

			queue_create_infos.push_back(queue_create_info);
			queue_indices.insert(queue_family.second);
		}
	}

	// initialise the logical device
	{
		VkPhysicalDeviceFeatures features{ };
		features.fillModeNonSolid = VK_TRUE;

		debugLog("    creating logical device...");
		VkDeviceCreateInfo device_create_info{ };
		device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		debugLog("        enabling " + to_string(queue_create_infos.size()) + " device queues");
		device_create_info.pQueueCreateInfos = queue_create_infos.data();
		device_create_info.queueCreateInfoCount = static_cast<uint32_t>(queue_create_infos.size());

		device_create_info.pEnabledFeatures = &features;

		debugLog("        enabling " + to_string(sizeof(required_device_extensions) / sizeof(required_device_extensions[0])) + " device extensions");
		device_create_info.enabledExtensionCount = sizeof(required_device_extensions) / sizeof(required_device_extensions[0]);
		device_create_info.ppEnabledExtensionNames = required_device_extensions;
#ifdef NDEBUG
		device_create_info.enabledLayerCount = 0;
#else
		debugLog("        enabling " + to_string(layers.size()) + " layers");
		device_create_info.enabledLayerCount = static_cast<uint32_t>(layers.size());
		device_create_info.ppEnabledLayerNames = layers.data();
#endif

		if (vkCreateDevice(physical_device.getDevice(), &device_create_info, nullptr, &device) != VK_SUCCESS)
			throw runtime_error("unable to create device");
		debugLog("    done.");
	}

	// grab the queue handles from the physical device
	{
		debugLog("    grabbing queue handles:");
		VkQueue queue;
		queues.clear();
		for (pair<PTPhysicalDevice::QueueFamily, uint32_t> queue_family : physical_device.getAllQueueFamilies())
		{
			vkGetDeviceQueue(device, queue_family.second, 0, &queue);
			queues.insert(make_pair(queue_family.first, queue));
			debugLog("       " + to_string(queue_family.second));
    	}
	}
}

void PTRenderServer::createCommandPoolAndBuffers()
{
	VkCommandPoolCreateInfo pool_create_info{ };
    pool_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    pool_create_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    pool_create_info.queueFamilyIndex = physical_device.getQueueFamily(PTPhysicalDevice::QueueFamily::GRAPHICS);

    if (vkCreateCommandPool(device, &pool_create_info, nullptr, &command_pool))
        throw std::runtime_error("unable to create command pool");

    VkCommandBufferAllocateInfo buffer_alloc_info{ };
    buffer_alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    buffer_alloc_info.commandPool = command_pool;
    buffer_alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    buffer_alloc_info.commandBufferCount = MAX_FRAMES_IN_FLIGHT;
    
    command_buffers.resize(MAX_FRAMES_IN_FLIGHT);

    if (vkAllocateCommandBuffers(device, &buffer_alloc_info, command_buffers.data()) != VK_SUCCESS)
        throw std::runtime_error("unable to allocate command buffers");
}

void PTRenderServer::createDescriptorPoolAndSets()
{
	// allow enough for a lot of indiviual uniform descriptors
	VkDescriptorPoolSize uniform_pool_size{ };
    uniform_pool_size.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uniform_pool_size.descriptorCount = MAX_FRAMES_IN_FLIGHT * MAX_OBJECTS * 16;

	// each object will have MAX_FRAMES_IN_FLIGHT descriptor sets associated probably
    VkDescriptorPoolCreateInfo pool_create_info{ };
    pool_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_create_info.maxSets = MAX_FRAMES_IN_FLIGHT * MAX_OBJECTS;
    pool_create_info.poolSizeCount = 1;
    pool_create_info.pPoolSizes = &uniform_pool_size;
    pool_create_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

	if (vkCreateDescriptorPool(device, &pool_create_info, nullptr, &descriptor_pool) != VK_SUCCESS)
		throw runtime_error("unable to create descriptor pool");

	// create a scene uniform buffer for each frame
    VkDeviceSize buffer_size = sizeof(SceneUniforms);
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        scene_uniform_buffers[i] = PTResourceManager::get()->createBuffer(buffer_size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
}

void PTRenderServer::createFramebufferAndSyncResources()
{
	// create depth and normal images
	{
		debugLog("creating Gbuffer resources...");
		depth_image = PTResourceManager::get()->createImage(swapchain->getExtent(), VK_FORMAT_D32_SFLOAT, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		depth_image_view = depth_image->createImageView(VK_IMAGE_ASPECT_DEPTH_BIT);
		debugLog("    created depth image and view");
		
		normal_image = PTResourceManager::get()->createImage(swapchain->getExtent(), VK_FORMAT_R16G16B16A16_SNORM, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		normal_image_view = normal_image->createImageView(VK_IMAGE_ASPECT_COLOR_BIT);
		debugLog("    created normal image and view");
		debugLog("done.");
	}

	// create new framebuffers, linked to the image views we just created
	{
		debugLog("creating framebuffers...");
		VkFramebufferCreateInfo framebuffer_create_info{ };
		framebuffer_create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebuffer_create_info.renderPass = render_pass->getRenderPass();
		framebuffer_create_info.attachmentCount = 3;
		framebuffer_create_info.width = swapchain->getExtent().width;
		framebuffer_create_info.height = swapchain->getExtent().height;
		framebuffer_create_info.layers = 1;

		framebuffers.resize(swapchain->getImageCount());
		for (uint32_t i = 0; i < swapchain->getImageCount(); i++)
		{
			VkImageView attachments[] = { swapchain->getImageView(i), normal_image_view, depth_image_view };
			
			framebuffer_create_info.pAttachments = attachments;
		
			if (vkCreateFramebuffer(device, &framebuffer_create_info, nullptr, &framebuffers[i]) != VK_SUCCESS)
				throw std::runtime_error("unable to create framebuffer");
    	}
		debugLog("    created " + to_string(framebuffers.size()) + " framebuffer objects");
		debugLog("done.");
	}

	// create framebuffer image sync resources
	{
		debugLog("creating sync resources...");
		VkSemaphoreCreateInfo semaphore_create_info{ };
		semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		
		VkFenceCreateInfo fence_create_info{ };
		fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fence_create_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
		
		image_available_semaphores.resize(MAX_FRAMES_IN_FLIGHT);
		render_finished_semaphores.resize(MAX_FRAMES_IN_FLIGHT);
		in_flight_fences.resize(MAX_FRAMES_IN_FLIGHT);
		
		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			if (vkCreateSemaphore(device, &semaphore_create_info, nullptr, &image_available_semaphores[i]) != VK_SUCCESS)
            	throw std::runtime_error("unable to create semaphore");
			
			if (vkCreateSemaphore(device, &semaphore_create_info, nullptr, &render_finished_semaphores[i]) != VK_SUCCESS)
           		throw std::runtime_error("unable to create semaphore");
			
			if (vkCreateFence(device, &fence_create_info, nullptr, &in_flight_fences[i]) != VK_SUCCESS)
            	throw std::runtime_error("unable to create fence");
		}
		debugLog("done.");
	}
}

void PTRenderServer::destroyFramebufferAndSyncResources()
{
	debugLog("destroying framebuffer and sync resources...");
    vkDeviceWaitIdle(device);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        vkDestroySemaphore(device, image_available_semaphores[i], nullptr);
        vkDestroySemaphore(device, render_finished_semaphores[i], nullptr);
        vkDestroyFence(device, in_flight_fences[i], nullptr);
    }

    for (auto framebuffer : framebuffers)
        vkDestroyFramebuffer(device, framebuffer, nullptr);
    framebuffers.clear();

    vkDestroyImageView(device, depth_image_view, nullptr);
    depth_image->removeReferencer();

    vkDestroyImageView(device, normal_image_view, nullptr);
    normal_image->removeReferencer();
	debugLog("done.");
}

VkResult PTRenderServer::createDebugUtilsMessenger(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, VkDebugUtilsMessengerEXT* pDebugMessenger)
{
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr)
        return func(instance, pCreateInfo, nullptr, pDebugMessenger);
    else
        return VK_ERROR_EXTENSION_NOT_PRESENT;
}

void PTRenderServer::destroyDebugUtilsMessenger(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger)
{
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr)
        func(instance, debugMessenger, nullptr);
}

void PTRenderServer::updateSceneAndTransformUniforms(uint32_t frame_index)
{
	{
        // update transforms of each draw request
        TransformUniforms uniforms;

        PTMatrix4f world_to_view;
        PTMatrix4f view_to_clip;
        PTApplication::get()->getCameraMatrix(world_to_view, view_to_clip);
        world_to_view.getColumnMajor(uniforms.world_to_view);
        view_to_clip.getColumnMajor(uniforms.view_to_clip);

        for (auto instruction : draw_queue)
        {
            instruction.second.transform->getLocalToWorld().getColumnMajor(uniforms.model_to_world);
            uniforms.object_id = (uint32_t)((size_t)instruction.first);

            memcpy(instruction.second.descriptor_buffers[frame_index]->map(), &uniforms, instruction.second.descriptor_buffers[frame_index]->getSize());
        }
    }

    {
        // update scene uniforms
        SceneUniforms uniforms;

        uniforms.viewport_size = PTVector2f{ (float)swapchain->getExtent().width, (float)swapchain->getExtent().height };
        uniforms.time = PTApplication::get()->getTotalTime();

        memcpy(scene_uniform_buffers[frame_index]->map(), &uniforms, scene_uniform_buffers[frame_index]->getSize());
    }
}

void PTRenderServer::drawFrame(uint32_t frame_index)
{
	updateSceneAndTransformUniforms(frame_index);

    vkWaitForFences(device, 1, &in_flight_fences[frame_index], VK_TRUE, UINT64_MAX);

    uint32_t image_index;
    VkResult result = vkAcquireNextImageKHR(device, swapchain->getSwapchain(), UINT64_MAX, image_available_semaphores[frame_index], VK_NULL_HANDLE, &image_index);

    if (result == VK_ERROR_OUT_OF_DATE_KHR)
    {
        debugLog("swapchain out of date during acquire image!");
        resizeSwapchain();
        return;
    }
    else if (window_resized)
    {
        debugLog("window resized during acquire image!");
        resizeSwapchain();
        return;
    }
    else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
        throw runtime_error("unable to acquire next swapchain image");

    vkResetFences(device, 1, &in_flight_fences[frame_index]);

    // before we start drawing, make a list of draw requests, sorted by priority, then by material, then by mesh
    vector<DrawRequest> sorted_queue;
    sorted_queue.reserve(draw_queue.size());
    for (auto p : draw_queue)
        sorted_queue.push_back(p.second);
    sort(sorted_queue.begin(), sorted_queue.end(), DrawRequest::compare);

    vkResetCommandBuffer(command_buffers[frame_index], 0);

    VkCommandBufferBeginInfo command_buffer_begin_info{ };
    command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    command_buffer_begin_info.flags = 0;
    command_buffer_begin_info.pInheritanceInfo = nullptr;

    VkViewport viewport{ };
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(swapchain->getExtent().width);
    viewport.height = static_cast<float>(swapchain->getExtent().height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor{ };
    scissor.offset = {0, 0};
    scissor.extent = swapchain->getExtent();

    VkRenderPassBeginInfo render_pass_begin_info{ };
    render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    render_pass_begin_info.renderPass = render_pass->getRenderPass();
    render_pass_begin_info.framebuffer = framebuffers[image_index];
    render_pass_begin_info.renderArea.offset = { 0, 0 };
    render_pass_begin_info.renderArea.extent = swapchain->getExtent();
    array<VkClearValue, 3> clear_values{ };
    clear_values[0].color = { { 1.0f, 0.0f, 1.0f, 1.0f } };
    clear_values[1].color = { { 0.0f, 0.0f, 0.0f, 1.0f } };
    clear_values[2].depthStencil = { 1.0f, 0 };
    render_pass_begin_info.clearValueCount = static_cast<uint32_t>(clear_values.size());
    render_pass_begin_info.pClearValues = clear_values.data();

    beginDrawLock();
    
    if (vkBeginCommandBuffer(command_buffers[frame_index], &command_buffer_begin_info) != VK_SUCCESS)
        throw runtime_error("unable to begin recording command buffer");

    vkCmdSetViewport(command_buffers[frame_index], 0, 1, &viewport);
    vkCmdSetScissor(command_buffers[frame_index], 0, 1, &scissor);
    vkCmdBeginRenderPass(command_buffers[frame_index], &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

    PTMaterial* mat = nullptr;
    PTMesh* mesh = nullptr;
    for (auto instruction : sorted_queue)
    {
        if (instruction.material != mat)
        {
            // for each material, bind the shader and pipeline, and the material-specific descriptor set
            mat = instruction.material;
            vkCmdBindPipeline(command_buffers[frame_index], VK_PIPELINE_BIND_POINT_GRAPHICS, mat->getPipeline()->getPipeline());
        }

        if (instruction.mesh != mesh)
        {
            // for each mesh, bind the vertex and index buffers
            mesh = instruction.mesh;
            VkBuffer vertex_buffers[] = { mesh->getVertexBuffer() };
            VkDeviceSize offsets[] = { 0 };
            vkCmdBindVertexBuffers(command_buffers[frame_index], 0, 1, vertex_buffers, offsets);
            vkCmdBindIndexBuffer(command_buffers[frame_index], mesh->getIndexBuffer(), 0, VK_INDEX_TYPE_UINT16);
        }

        // for each object, bind the object-specific common descriptor set, then draw indexed
        vkCmdBindDescriptorSets(command_buffers[frame_index], VK_PIPELINE_BIND_POINT_GRAPHICS, mat->getPipeline()->getLayout(), 0, 1, &(instruction.descriptor_sets[frame_index]), 0, nullptr);
        vkCmdDrawIndexed(command_buffers[frame_index], static_cast<uint32_t>(mesh->getIndexCount()), 1, 0, 0, 0);
    }

    vkCmdEndRenderPass(command_buffers[frame_index]);

    if (vkEndCommandBuffer(command_buffers[frame_index]) != VK_SUCCESS)
        throw std::runtime_error("unable to record command buffer");

    VkSubmitInfo submit_info{ };
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    VkSemaphore submit_wait_semaphores[] = { image_available_semaphores[frame_index] };
    VkPipelineStageFlags submit_wait_stages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores = submit_wait_semaphores;
    submit_info.pWaitDstStageMask = submit_wait_stages;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &command_buffers[frame_index];
    VkSemaphore signal_semaphores[] = { render_finished_semaphores[frame_index] };
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores = signal_semaphores;

    if (vkQueueSubmit(queues[PTPhysicalDevice::QueueFamily::GRAPHICS], 1, &submit_info, in_flight_fences[frame_index]) != VK_SUCCESS)
        throw std::runtime_error("unable to submit draw command buffer");

    VkPresentInfoKHR present_info{ };
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores = signal_semaphores;
    VkSwapchainKHR swap_chains[] = { swapchain->getSwapchain() };
    present_info.swapchainCount = 1;
    present_info.pSwapchains = swap_chains;
    present_info.pImageIndices = &image_index;
    present_info.pResults = nullptr;

    result = vkQueuePresentKHR(queues[PTPhysicalDevice::QueueFamily::PRESENT], &present_info);
    if (result == VK_ERROR_OUT_OF_DATE_KHR)
    {
        debugLog("swapchain out of date during present~");
        resizeSwapchain();
    }
    else if (window_resized)
    {
        debugLog("window resized during present~");
        resizeSwapchain();
    }
    else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
        throw runtime_error("unable to present swapchain image");

    vkWaitForFences(device, 1, &in_flight_fences[frame_index], VK_TRUE, UINT64_MAX);
    endDrawLock();
}

void PTRenderServer::resizeSwapchain()
{
	debugLog("resizing swapchain + framebuffer...");
    destroyFramebufferAndSyncResources();

	// keep checking until it actually makes sense
	PTVector2u size{ 0, 0 };
    while (size.x == 0 || size.y == 0)
        size = PTApplication::get()->getFramebufferSize();

    debugLog("    new size: " + to_string(size.x) + ", " + to_string(size.y));
    swapchain->resize(surface, size.x, size.y);   // tooootally doesnt completely recreate the swapchain....

    createFramebufferAndSyncResources();

    window_resized = false;
    vkDeviceWaitIdle(device);
    
    debugLog("done.");
}

void PTRenderServer::takeScreenshot(uint32_t frame_index)
{
	VkCommandBuffer cmd = beginTransientCommands();

    VkExtent2D ext = swapchain->getExtent();
    PTImage* screenshot_img = PTResourceManager::get()->createImage(ext, 
                                                                    VK_FORMAT_R8G8B8A8_SRGB, 
                                                                    VK_IMAGE_TILING_OPTIMAL, 
                                                                    VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, 
                                                                    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    screenshot_img->transitionImageLayout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, cmd);
    VkMemoryRequirements memory_requirements{ };
    vkGetImageMemoryRequirements(device, screenshot_img->getImage(), &memory_requirements);
    PTBuffer* host_buffer = PTResourceManager::get()->createBuffer(memory_requirements.size, VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    VkImageSubresourceLayers src_layers{ };
    src_layers.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    src_layers.baseArrayLayer = 0;
    src_layers.layerCount = 1;
    src_layers.mipLevel = 0;

    VkImageBlit blit_region{ };
    blit_region.srcOffsets[0] = VkOffset3D{ 0, 0, 0 };
    blit_region.srcOffsets[1] = VkOffset3D{ static_cast<int32_t>(ext.width), static_cast<int32_t>(ext.height), 1 };
    blit_region.dstOffsets[0] = VkOffset3D{ 0, static_cast<int32_t>(ext.height), 0 };
    blit_region.dstOffsets[1] = VkOffset3D{ static_cast<int32_t>(ext.width), 0, 1 };
    blit_region.srcSubresource = src_layers;
    blit_region.dstSubresource = src_layers;
    
    VkImageMemoryBarrier image_barrier{ };
    image_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    image_barrier.image = swapchain->getImage(frame_index);
    image_barrier.oldLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    image_barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    image_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    image_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    image_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    image_barrier.subresourceRange.baseMipLevel = 0;
    image_barrier.subresourceRange.levelCount = 1;
    image_barrier.subresourceRange.baseArrayLayer = 0;
    image_barrier.subresourceRange.layerCount = 1;
    image_barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    image_barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

    vkCmdPipelineBarrier
    (
        cmd,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
        0,
        0, nullptr,
        0, nullptr,
        1, &image_barrier
    );
    
    vkCmdBlitImage(cmd, swapchain->getImage(frame_index), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, screenshot_img->getImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit_region, VkFilter::VK_FILTER_NEAREST);
    
    VkImageMemoryBarrier image_barrier_reversed = image_barrier;
    image_barrier_reversed.oldLayout = image_barrier.newLayout;
    image_barrier_reversed.newLayout = image_barrier.oldLayout;
    image_barrier_reversed.srcAccessMask = image_barrier.dstAccessMask;
    image_barrier_reversed.dstAccessMask = image_barrier.srcAccessMask;
    
    vkCmdPipelineBarrier
    (
        cmd,
        VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        0,
        0, nullptr,
        0, nullptr,
        1, &image_barrier_reversed
    );

    VkBufferImageCopy copy_region{ };
    copy_region.imageOffset = VkOffset3D{ 0, 0, 0 };
    copy_region.imageExtent = VkExtent3D{ ext.width, ext.height, 1 };
    copy_region.imageSubresource = src_layers;
    copy_region.bufferOffset = 0;
    copy_region.bufferRowLength = ext.width;
    copy_region.bufferImageHeight = ext.height;

    screenshot_img->transitionImageLayout(VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, cmd);

    vkCmdCopyImageToBuffer(cmd, screenshot_img->getImage(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, host_buffer->getBuffer(), 1, &copy_region);

    endTransientCommands(cmd);

    screenshot_img->removeReferencer();

    writeRGBABitmap("screenshot.bmp", host_buffer->map(), ext.width, ext.height);

    host_buffer->removeReferencer();
    wants_screenshot = false;
}

PTPhysicalDevice PTRenderServer::selectPhysicalDevice()
{
    debugLog("    enumerating physical devices: ");
    vector<PTPhysicalDevice> devices = PTPhysicalDevice::enumerateDevices(instance, surface);
    debugLog("    " + to_string(devices.size()) + " found.");
    debugLog("    selecting physical device...");
    if (devices.size() == 0)
        throw runtime_error("unable to find physical device");

    multimap<int, PTPhysicalDevice> device_scores;
    for (PTPhysicalDevice dev : devices)
        device_scores.insert(make_pair(evaluatePhysicalDevice(dev), dev));

    // select physical device
    if (device_scores.rbegin()->first <= 0)
        throw runtime_error("no valid physical device found");

    debugLog("        selected device: \'" + string(physical_device.getProperties().deviceName) + "\' (" + to_string(physical_device.getProperties().deviceID) + ")" + " with score " + to_string(device_scores.rbegin()->first));
    debugLog("    done.");

    return device_scores.rbegin()->second;
}

int PTRenderServer::evaluatePhysicalDevice(PTPhysicalDevice d)
{
    int score = 0;

    // this device is unsuitable if it doesn't have all the queues we want
    if (!d.hasQueueFamily(PTPhysicalDevice::QueueFamily::GRAPHICS))
        return 0;
    if (!d.hasQueueFamily(PTPhysicalDevice::QueueFamily::PRESENT))
        return 0;

    // if any required extensions are absent, the device is unsuitable
    for (const char* extension_name : required_device_extensions)
    {
        bool found = false;
        for (VkExtensionProperties& prop : d.getExtensions())
        {
            if (strcmp(prop.extensionName, extension_name) == 0)
            {
                found = true;
                break;
            }
        }
        if (!found)
            return 0;
    }

    // query swapchain support
    if (d.getSwapchainFormats().size() == 0)
        return 0;
    if (d.getSwapchainPresentModes().size() == 0)
        return 0;

    // award points for being a good boy
    auto device_properties = d.getProperties();
    auto device_features = d.getFeatures();
    if (device_properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
        score += 10000;
    if (device_properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)
        score += 6000;
    if (device_properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_CPU)
        score += 4000;
    if (device_properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU)
        score += 2000;
    if (device_features.geometryShader == VK_TRUE)
        score += 10;
    if (device_features.fillModeNonSolid == VK_TRUE)
        score += 10;
    else
        return 0;

    return score;
}

bool PTRenderServer::DrawRequest::compare(const DrawRequest& a, const DrawRequest& b)
{
    if (a.material->getPriority() > b.material->getPriority())
        return true;
    if (a.material->getPriority() < b.material->getPriority())
        return false;

    return (a.material < b.material) && (a.mesh < b.mesh);
}