#include "application.h"

#include <stdexcept>
#include <vector>
#include <cstring>
#include <iostream>
#include <set>
#include <algorithm>
#include <cstdlib>

#include "scene.h"
#include "debug.h"
#include "mesh.h"
#include "resource_manager.h"

using namespace std;

static PTApplication* main_application = nullptr;

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

PTApplication::PTApplication(unsigned int _width, unsigned int _height)
{
    width = _width;
    height = _height;
}

void PTApplication::start()
{
    main_application = this;

    initWindow();
    initVulkan();
    initController();

    demo_mesh = PTResourceManager::get()->createMesh("suzanne.obj");

    auto tokens = PTDeserialiser::prune(PTDeserialiser::tokenise(demo));
    size_t start = 0;
    std::map<std::string, PTResource*> resources;
    auto res_desc = PTDeserialiser::deserialiseResourceDescriptor(tokens, start, resources, demo);

    mainLoop();

    deinitController();

    deinitVulkan();
    deinitWindow();
}

PTInputManager* PTApplication::getInputManager()
{
    return input_manager;
}

PTApplication* PTApplication::get()
{
    return main_application;
}

void PTApplication::initWindow()
{
    debugLog("initialising glfw...");
    glfwInit();

    debugLog("    initialising window...");
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    window = glfwCreateWindow(width, height, "planetarium", nullptr, nullptr);
    glfwSetFramebufferSizeCallback(window, windowResizeCallback);

    debugLog("done.");
}

void PTApplication::initVulkan()
{
    debugLog("initialising vulkan...");

    // initialise vulkan app instance
    vector<const char*> layers;
    initVulkanInstance(layers);

    // create surface (target to render to)
    initSurface();

    // scan physical devices
    physical_device = selectPhysicalDevice();

    // create queue indices
    vector<VkDeviceQueueCreateInfo> queue_create_infos = constructQueues();

    // create logical device
    initLogicalDevice(queue_create_infos, layers);

    PTResourceManager::init(device, physical_device);

    // grab queue handles
    collectQueues();

    // create swap chain
    int width;
    int height;
    glfwGetFramebufferSize(window, &width, &height);
    debugLog("    creating swapchain...");
    swapchain = PTResourceManager::get()->createSwapchain(surface, width, height);
    debugLog("    done.");

    // initialise a basic pipeline
    debugLog("    loading demo shader...");
    demo_shader = PTResourceManager::get()->createShader("demo");
    debugLog("    done.");
    debugLog("    creating demo render pass...");
    PTRenderPassAttachment attachment;
    attachment.format = swapchain->getImageFormat();
    demo_render_pass = PTResourceManager::get()->createRenderPass({ attachment }, true);
    debugLog("    done.");
    debugLog("    constructing pipelines...");
    demo_pipeline = PTResourceManager::get()->createPipeline(demo_shader, demo_render_pass, swapchain, VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE, VK_POLYGON_MODE_FILL, { });
    debug_pipeline = PTResourceManager::get()->createPipeline(demo_shader, demo_render_pass, swapchain, VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS, VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE, VK_POLYGON_MODE_LINE, { });
    debugLog("    done.");

    createCommandPoolAndBuffers();

    createDepthResources();

    createFramebuffers(demo_render_pass->getRenderPass());

    createUniformBuffers();

    createDescriptorPoolAndSets();

    createSyncObjects();

    debugLog("done.");
}

void PTApplication::initController()
{
    glfwSetKeyCallback(window, keyboardCallback);
    input_manager = new PTInputManager();
    initInputManager(input_manager);
}

void PTApplication::mainLoop()
{
    current_scene = new PTScene();

    int frame_total_number = 0;
    uint32_t frame_index = 0;
    last_frame_start = chrono::high_resolution_clock::now();
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        input_manager->pollControllers();

        auto now = chrono::high_resolution_clock::now();
        chrono::duration<float> frame_time = now - last_frame_start;

        frame_time_running_mean_us = (frame_time_running_mean_us + (chrono::duration_cast<chrono::microseconds>(frame_time).count())) / 2;

        debugFrametiming(frame_time_running_mean_us / 1000.0f, frame_total_number);        
        last_frame_start = now;

        current_scene->update(frame_time.count());

        drawFrame(frame_index);

        frame_index = (frame_index + 1) % MAX_FRAMES_IN_FLIGHT;
        frame_total_number++;
    }

    vkDeviceWaitIdle(device);
}

void PTApplication::deinitController()
{
}

void PTApplication::deinitVulkan()
{
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        vkDestroySemaphore(device, image_available_semaphores[i], nullptr);
        vkDestroySemaphore(device, render_finished_semaphores[i], nullptr);
        vkDestroyFence(device, in_flight_fences[i], nullptr);
    }
    
    vkDestroyCommandPool(device, command_pool, nullptr);

    for (auto framebuffer : framebuffers)
        vkDestroyFramebuffer(device, framebuffer, nullptr);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        uniform_buffers[i]->removeReferencer();
    }
    uniform_buffers.clear();

    vkDestroyDescriptorPool(device, descriptor_pool, nullptr);

    debug_pipeline->removeReferencer();
    demo_pipeline->removeReferencer();
    demo_shader->removeReferencer();
    demo_render_pass->removeReferencer();

    swapchain->removeReferencer();

    demo_mesh->removeReferencer();

    vkDestroyImageView(device, depth_image_view, nullptr);
    depth_image->removeReferencer();

    PTResourceManager::deinit();

    vkDestroyDevice(device, nullptr);

    vkDestroySurfaceKHR(instance, surface, nullptr);

#ifndef NDEBUG
    destroyDebugUtilsMessenger(instance, debug_messenger, nullptr);
#endif

    vkDestroyInstance(instance, nullptr);
}

void PTApplication::deinitWindow()
{
    glfwDestroyWindow(window);

    glfwTerminate();
}

void PTApplication::initVulkanInstance(vector<const char*>& layers)
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
    vector<const char*> extensions = getRequiredExtensions();
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

    if (createDebugUtilsMessenger(instance, &debug_messenger_create_info, nullptr, &debug_messenger) != VK_SUCCESS)
        throw runtime_error("unable to set up debug messenger!");
#endif
    debugLog("    done.");
}

void PTApplication::initSurface()
{
    debugLog("    creating window surface...");
    if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS)
        throw runtime_error("unable to create window surface");
    debugLog("    done.");
}

PTPhysicalDevice PTApplication::selectPhysicalDevice()
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

vector<VkDeviceQueueCreateInfo> PTApplication::constructQueues()
{
    vector<VkDeviceQueueCreateInfo> queue_create_infos;
    set<uint32_t> queue_indices;
    float graphics_queue_priority = 1.0f;
    for (pair<PTQueueFamily, uint32_t> queue_family : physical_device.getAllQueueFamilies())
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

    return queue_create_infos;
}

void PTApplication::initLogicalDevice(const vector<VkDeviceQueueCreateInfo>& queue_create_infos, const vector<const char*>& layers)
{
    VkPhysicalDeviceFeatures features{ };
    features.fillModeNonSolid = VK_TRUE;

    debugLog("    creating logical device...");
    VkDeviceCreateInfo device_create_info{ };
    device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    debugLog("        enabling " + to_string(queue_create_infos.size()) + " device queues");
    device_create_info.pQueueCreateInfos = queue_create_infos.data();
    device_create_info.queueCreateInfoCount = queue_create_infos.size();

    device_create_info.pEnabledFeatures = &features;

    debugLog("        enabling " + to_string(sizeof(required_device_extensions) / sizeof(required_device_extensions[0])) + " device extensions");
    device_create_info.enabledExtensionCount = sizeof(required_device_extensions) / sizeof(required_device_extensions[0]);
    device_create_info.ppEnabledExtensionNames = required_device_extensions;
#ifdef NDEBUG
    device_create_info.enabledLayerCount = 0;
#else
    debugLog("        enabling " + to_string(layers.size()) + " layers");
    device_create_info.enabledLayerCount = layers.size();
    device_create_info.ppEnabledLayerNames = layers.data();
#endif

    if (vkCreateDevice(physical_device.getDevice(), &device_create_info, nullptr, &device) != VK_SUCCESS)
        throw runtime_error("unable to create device");
    debugLog("    done.");
}

void PTApplication::collectQueues()
{
    debugLog("    grabbing queue handles:");
    VkQueue queue;
    queues.clear();
    for (pair<PTQueueFamily, uint32_t> queue_family : physical_device.getAllQueueFamilies())
    {
        vkGetDeviceQueue(device, queue_family.second, 0, &queue);
        queues.insert(make_pair(queue_family.first, queue));
        debugLog("       " + to_string(queue_family.second));
    }
}

void PTApplication::createFramebuffers(const VkRenderPass render_pass)
{
    debugLog("    creating framebuffers...");

    framebuffers.resize(swapchain->getImageCount());

    for (size_t i = 0; i < swapchain->getImageCount(); i++)
    {
        VkImageView attachments[] = { swapchain->getImageView(i), depth_image_view };

        VkFramebufferCreateInfo framebuffer_create_info{ };
        framebuffer_create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebuffer_create_info.renderPass = render_pass;
        framebuffer_create_info.attachmentCount = 2;
        framebuffer_create_info.pAttachments = attachments;
        framebuffer_create_info.width = swapchain->getExtent().width;
        framebuffer_create_info.height = swapchain->getExtent().height;
        framebuffer_create_info.layers = 1;

        if (vkCreateFramebuffer(device, &framebuffer_create_info, nullptr, &framebuffers[i]) != VK_SUCCESS)
            throw std::runtime_error("unable to create framebuffer");
    }

    debugLog("        created " + to_string(framebuffers.size()) + " framebuffer objects");
    debugLog("    done.");
}

void PTApplication::createUniformBuffers()
{
    VkDeviceSize transform_buffer_size = sizeof(TransformMatrices);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        PTBuffer* buf = PTResourceManager::get()->createBuffer(transform_buffer_size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        buf->map();
        uniform_buffers.push_back(buf);
    }
}

void PTApplication::createCommandPoolAndBuffers()
{
    VkCommandPoolCreateInfo pool_create_info{ };
    pool_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    pool_create_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    pool_create_info.queueFamilyIndex = physical_device.getQueueFamily(PTQueueFamily::GRAPHICS);

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

void PTApplication::createDepthResources()
{
    depth_image = PTResourceManager::get()->createImage(swapchain->getExtent(), VK_FORMAT_D32_SFLOAT, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    depth_image_view = depth_image->createImageView(VK_IMAGE_ASPECT_DEPTH_BIT);
}

void PTApplication::createDescriptorPoolAndSets()
{
    VkDescriptorPoolSize pool_size{ };
    pool_size.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    pool_size.descriptorCount = MAX_FRAMES_IN_FLIGHT;

    VkDescriptorPoolCreateInfo pool_create_info{ };
    pool_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_create_info.maxSets = MAX_FRAMES_IN_FLIGHT;
    pool_create_info.poolSizeCount = 1;
    pool_create_info.pPoolSizes = &pool_size;

    if (vkCreateDescriptorPool(device, &pool_create_info, nullptr, &descriptor_pool) != VK_SUCCESS)
        throw runtime_error("unable to create descriptor pool");

    vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, demo_shader->getDescriptorSetLayout());
    VkDescriptorSetAllocateInfo set_allocation_info{ };
    set_allocation_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    set_allocation_info.descriptorPool = descriptor_pool;
    set_allocation_info.descriptorSetCount = MAX_FRAMES_IN_FLIGHT;
    set_allocation_info.pSetLayouts = layouts.data();

    descriptor_sets.resize(MAX_FRAMES_IN_FLIGHT);
    if (vkAllocateDescriptorSets(device, &set_allocation_info, descriptor_sets.data()) != VK_SUCCESS)
        throw runtime_error("unable to allocate descriptor sets");
    
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        VkDescriptorBufferInfo buffer_info{ };
        buffer_info.buffer = uniform_buffers[i]->getBuffer();
        buffer_info.offset = 0;
        buffer_info.range = sizeof(TransformMatrices);

        VkWriteDescriptorSet write_set{ };
        write_set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write_set.dstSet = descriptor_sets[i];
        write_set.dstBinding = 0;
        write_set.dstArrayElement = 0;
        write_set.descriptorCount = 1;
        write_set.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        write_set.pBufferInfo = &buffer_info;

        vkUpdateDescriptorSets(device, 1, &write_set, 0, nullptr);
    }
}

void PTApplication::createSyncObjects()
{
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
}

void PTApplication::drawFrame(uint32_t frame_index)
{
    updateUniformBuffers(frame_index);

        // TODO: move the following into appropriate functions, this is just a test
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
            resizeSwapchain();
            return;
        }
        else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
            throw runtime_error("unable to acquire next swapchain image");

        vkResetFences(device, 1, &in_flight_fences[frame_index]);

        vkResetCommandBuffer(command_buffers[frame_index], 0);

        VkCommandBufferBeginInfo command_buffer_begin_info{ };
        command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        command_buffer_begin_info.flags = 0;
        command_buffer_begin_info.pInheritanceInfo = nullptr;

        if (vkBeginCommandBuffer(command_buffers[frame_index], &command_buffer_begin_info) != VK_SUCCESS)
            throw std::runtime_error("unable to begin recording command buffer");

        VkRenderPassBeginInfo render_pass_begin_info{ };
        render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        render_pass_begin_info.renderPass = demo_render_pass->getRenderPass();
        render_pass_begin_info.framebuffer = framebuffers[image_index];
        render_pass_begin_info.renderArea.offset = { 0, 0 };
        render_pass_begin_info.renderArea.extent = swapchain->getExtent();
        array<VkClearValue, 2> clear_values{ };
        clear_values[0].color = { { 1.0f, 0.0f, 1.0f, 1.0f } };
        clear_values[1].depthStencil = { 1.0f, 0 };
        render_pass_begin_info.clearValueCount = static_cast<uint32_t>(clear_values.size());
        render_pass_begin_info.pClearValues = clear_values.data();

        PTPipeline* pipeline = debug_mode ? debug_pipeline : demo_pipeline;

        vkCmdBeginRenderPass(command_buffers[frame_index], &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdBindPipeline(command_buffers[frame_index], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->getPipeline());

        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = static_cast<float>(swapchain->getExtent().width);
        viewport.height = static_cast<float>(swapchain->getExtent().height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(command_buffers[frame_index], 0, 1, &viewport);

        VkRect2D scissor{};
        scissor.offset = {0, 0};
        scissor.extent = swapchain->getExtent();
        vkCmdSetScissor(command_buffers[frame_index], 0, 1, &scissor);

        VkBuffer vertex_buffers[] = { demo_mesh->getVertexBuffer() };
        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(command_buffers[frame_index], 0, 1, vertex_buffers, offsets);
        vkCmdBindIndexBuffer(command_buffers[frame_index], demo_mesh->getIndexBuffer(), 0, VK_INDEX_TYPE_UINT16);
        vkCmdBindDescriptorSets(command_buffers[frame_index], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->getLayout(), 0, 1, &descriptor_sets[frame_index], 0, nullptr);
        vkCmdDrawIndexed(command_buffers[frame_index], static_cast<uint32_t>(demo_mesh->getIndexCount()), 1, 0, 0, 0);
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

        if (vkQueueSubmit(queues[PTQueueFamily::GRAPHICS], 1, &submit_info, in_flight_fences[frame_index]) != VK_SUCCESS)
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

        result = vkQueuePresentKHR(queues[PTQueueFamily::PRESENT], &present_info);
        if (result == VK_ERROR_OUT_OF_DATE_KHR)
        {
            debugLog("swapchain out of date during present~");
            // FIXME: need to signal the image_available semaphore regardless
            resizeSwapchain();
        }
        else if (window_resized)
        {
            resizeSwapchain();
        }
        else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
            throw runtime_error("unable to present swapchain image");
}

void PTApplication::resizeSwapchain()
{
    debugLog("resizing swapchain + framebuffer...");
    vkDeviceWaitIdle(device);
    debugLog("    old size: " + to_string(swapchain->getExtent().width) + ", " + to_string(swapchain->getExtent().height));

    for (auto framebuffer : framebuffers)
        vkDestroyFramebuffer(device, framebuffer, nullptr);

    swapchain->removeReferencer();

    vkDestroyImageView(device, depth_image_view, nullptr);
    depth_image->removeReferencer();

    glfwGetFramebufferSize(window, &width, &height);
    while (width == 0 || height == 0)
    {
        glfwGetFramebufferSize(window, &width, &height);
        glfwWaitEvents();
    }
    debugLog("    new size: " + to_string(width) + ", " + to_string(height));
    swapchain = PTResourceManager::get()->createSwapchain(surface, width, height);

    createDepthResources();
    createFramebuffers(demo_render_pass->getRenderPass());

    window_resized = false;

    debugLog("done.");
}

void PTApplication::windowResizeCallback(GLFWwindow* window, int new_width, int new_height)
{
    get()->window_resized = true;
}

VkCommandBuffer PTApplication::beginTransientCommands()
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

void PTApplication::endTransientCommands(VkCommandBuffer transient_command_buffer)
{
    vkEndCommandBuffer(transient_command_buffer);

    VkSubmitInfo submit_info{ };
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &transient_command_buffer;

    vkQueueSubmit(queues[PTQueueFamily::GRAPHICS], 1, &submit_info, VK_NULL_HANDLE);
    vkQueueWaitIdle(queues[PTQueueFamily::GRAPHICS]);

    vkFreeCommandBuffers(device, command_pool, 1, &transient_command_buffer);
}

void PTApplication::updateUniformBuffers(uint32_t frame_index)
{
    TransformMatrices transform;
    current_scene->getCameraMatrix().getColumnMajor(transform.world_to_clip);
    PTMatrix4f().getColumnMajor(transform.model_to_world);

    debugSetSceneProperty("w2c r0", to_string(current_scene->getCameraMatrix().row0()));
    debugSetSceneProperty("w2c r1", to_string(current_scene->getCameraMatrix().row1()));
    debugSetSceneProperty("w2c r2", to_string(current_scene->getCameraMatrix().row2()));
    debugSetSceneProperty("w2c r3", to_string(current_scene->getCameraMatrix().row3()));

    // cout << "blank matrix:" << endl;
    // cout << OLMatrix4f().row0() << endl;;
    // cout << OLMatrix4f().row1() << endl;;
    // cout << OLMatrix4f().row2() << endl;;
    // cout << OLMatrix4f().row3() << endl;;

    memcpy(uniform_buffers[frame_index]->getMappedMemory(), &transform, sizeof(TransformMatrices));
}

int PTApplication::evaluatePhysicalDevice(PTPhysicalDevice d)
{
    int score = 0;

    // this device is unsuitable if it doesn't have all the queues we want
    if (!d.hasQueueFamily(PTQueueFamily::GRAPHICS))
        return 0;
    if (!d.hasQueueFamily(PTQueueFamily::PRESENT))
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

vector<const char*> PTApplication::getRequiredExtensions()
{
    uint32_t glfw_extension_count = 0;
    const char** glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);

    vector<const char*> extensions(glfw_extensions, glfw_extensions + glfw_extension_count);

#ifndef NDEBUG
    extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif

    return extensions;
}

VkResult PTApplication::createDebugUtilsMessenger(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger)
{
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr)
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    else
        return VK_ERROR_EXTENSION_NOT_PRESENT;
}

void PTApplication::destroyDebugUtilsMessenger(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator)
{
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr)
        func(instance, debugMessenger, pAllocator);
}