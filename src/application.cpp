#include "application.h"

#include <stdexcept>
#include <vector>
#include <cstring>
#include <iostream>
#include <set>
#include <algorithm>
#include <cstdlib>
#include "../lib/oculib/image.h"

#include "scene.h"
#include "debug.h"

static PTApplication* main_application = nullptr;

PTApplication::PTApplication(unsigned int _width, unsigned int _height)
{
    width = _width;
    height = _height;
}

void PTApplication::start()
{
    main_application = this;
    demo_mesh = OLMesh("suzanne.obj");
    initWindow();
    initVulkan();
    initController();

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

    debugLog("done.");
}

void PTApplication::initVulkan()
{
    debugLog("initialising vulkan...");

    // initialise vulkan app instance
    vector<const char*> layers;
    initVulkanInstance(layers);

    // TODO: setup debug messenger

    // create surface (target to render to)
    initSurface();

    // scan physical devices
    physical_device = selectPhysicalDevice();

    // create queue indices
    vector<VkDeviceQueueCreateInfo> queue_create_infos = constructQueues();

    // create logical device
    initLogicalDevice(queue_create_infos, layers);

    // grab queue handles
    collectQueues();

    // create swap chain
    int width;
    int height;
    glfwGetFramebufferSize(window, &width, &height);
    debugLog("    creating swapchain...");
    swapchain = new PTSwapchain(width, height, physical_device, device, surface);
    debugLog("    done.");

    createDescriptorSetLayout();

    // initialise a basic pipeline
    demo_shader = new PTShader(device, "demo");
    demo_render_pass = createRenderPass();
    demo_pipeline = constructPipeline(*demo_shader, demo_render_pass);

    createCommandPoolAndBuffers();

    createDepthResources();

    createFramebuffers(demo_render_pass);

    createVertexBuffer();

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
    current_scene = new PTScene(this);

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

        debugFrametiming(chrono::duration_cast<chrono::nanoseconds>(frame_time).count() / 1000000.0f, frame_total_number);        
        last_frame_start = now;

        current_scene->update(frame_time.count());

        updateUniformBuffers(frame_index);

        // TODO: move the following into appropriate functions, this is just a test
        vkWaitForFences(device, 1, &in_flight_fences[frame_index], VK_TRUE, UINT64_MAX);

        uint32_t image_index;
        VkResult result = vkAcquireNextImageKHR(device, swapchain->getSwapchain(), UINT64_MAX, image_available_semaphores[frame_index], VK_NULL_HANDLE, &image_index);

        if (result == VK_ERROR_OUT_OF_DATE_KHR)
        {
            resizeSwapchain();
            continue;
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
        render_pass_begin_info.renderPass = demo_render_pass;
        render_pass_begin_info.framebuffer = framebuffers[image_index];
        render_pass_begin_info.renderArea.offset = { 0, 0 };
        render_pass_begin_info.renderArea.extent = swapchain->getExtent();
        array<VkClearValue, 2> clear_values{ };
        clear_values[0].color = { { 1.0f, 0.0f, 1.0f, 1.0f } };
        clear_values[1].depthStencil = { 1.0f, 0 };
        render_pass_begin_info.clearValueCount = static_cast<uint32_t>(clear_values.size());
        render_pass_begin_info.pClearValues = clear_values.data();

        vkCmdBeginRenderPass(command_buffers[frame_index], &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdBindPipeline(command_buffers[frame_index], VK_PIPELINE_BIND_POINT_GRAPHICS, demo_pipeline.pipeline);

        VkBuffer vertex_buffers[] = { vertex_buffer->getBuffer() };
        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(command_buffers[frame_index], 0, 1, vertex_buffers, offsets);
        vkCmdBindIndexBuffer(command_buffers[frame_index], index_buffer->getBuffer(), 0, VK_INDEX_TYPE_UINT16);
        vkCmdBindDescriptorSets(command_buffers[frame_index], VK_PIPELINE_BIND_POINT_GRAPHICS, demo_pipeline.layout, 0, 1, &descriptor_sets[frame_index], 0, nullptr);
        vkCmdDrawIndexed(command_buffers[frame_index], static_cast<uint32_t>(demo_mesh.indices.size()), 1, 0, 0, 0);
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
            resizeSwapchain();
        else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
            throw runtime_error("unable to present swapchain image");

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
        delete uniform_buffers[i];

    vkDestroyDescriptorPool(device, descriptor_pool, nullptr);

    vkDestroyDescriptorSetLayout(device, demo_descriptor_set_layout, nullptr);

    vkDestroyPipeline(device, demo_pipeline.pipeline, nullptr);
    vkDestroyPipelineLayout(device, demo_pipeline.layout, nullptr);
    delete demo_shader;
    vkDestroyRenderPass(device, demo_render_pass, nullptr);

    delete swapchain;

    delete index_buffer;
    delete vertex_buffer;

    vkDestroyImageView(device, depth_image_view, nullptr);
    vkDestroyImage(device, depth_image, nullptr);
    vkFreeMemory(device, depth_image_memory, nullptr);

    vkDestroyDevice(device, nullptr);

    vkDestroySurfaceKHR(instance, surface, nullptr);

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
    uint32_t glfw_extension_count = 0;
    const char** glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);
    create_info.enabledExtensionCount = glfw_extension_count;
    create_info.ppEnabledExtensionNames = glfw_extensions;
#ifdef NDEBUG
    create_info.enabledLayerCount = 0;
    create_info.ppEnabledLayerNames = nullptr;
#else
    layers.clear();
    layers.push_back(target_debug_layer);
    create_info.enabledLayerCount = layers.size();
    create_info.ppEnabledLayerNames = layers.data();
#endif
    debugLog("    extensions enabled:");
    for (size_t i = 0; i < glfw_extension_count; i++)
        debugLog("       " + string(glfw_extensions[i]));

    debugLog("    creating VK instance...");
    VkResult result = vkCreateInstance(&create_info, nullptr, &instance);
    if (result != VK_SUCCESS)
    {
        debugLog("    VK instance creation failed: " + string(/*string_VkResult(result) <<*/));
        throw runtime_error("unable to create VK instance");
    }
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
    // TODO: specify physical device features
    VkPhysicalDeviceFeatures features{ };

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

void PTApplication::createDescriptorSetLayout()
{
    VkDescriptorSetLayoutBinding transform_binding{ };
    transform_binding.binding = 0;
    transform_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    transform_binding.descriptorCount = 1;
    transform_binding.stageFlags = VK_SHADER_STAGE_ALL_GRAPHICS;

    VkDescriptorSetLayoutCreateInfo layout_create_info{ };
    layout_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layout_create_info.bindingCount = 1;
    layout_create_info.pBindings = &transform_binding;

    if (vkCreateDescriptorSetLayout(device, &layout_create_info, nullptr, &demo_descriptor_set_layout) != VK_SUCCESS)
        throw runtime_error("unable to create descriptor set layout");
    

}

VkRenderPass PTApplication::createRenderPass()
{
    debugLog("    creating render pass...");

    VkAttachmentDescription colour_attachment{ };
    colour_attachment.format = swapchain->getImageFormat();
    colour_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colour_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colour_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colour_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colour_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colour_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colour_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colour_attachment_ref{ };
    colour_attachment_ref.attachment = 0;
    colour_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

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
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colour_attachment_ref;
    subpass.pDepthStencilAttachment = &depth_attachment_ref;

    VkSubpassDependency dependency{ };
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    array<VkAttachmentDescription, 2> attachments = { colour_attachment, depth_attachment };
    VkRenderPassCreateInfo render_pass_create_info{ };
    render_pass_create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_create_info.attachmentCount = static_cast<uint32_t>(attachments.size());
    render_pass_create_info.pAttachments = attachments.data();
    render_pass_create_info.subpassCount = 1;
    render_pass_create_info.pSubpasses = &subpass;
    render_pass_create_info.dependencyCount = 1;
    render_pass_create_info.pDependencies = &dependency;

    VkRenderPass render_pass;

    if (vkCreateRenderPass(device, &render_pass_create_info, nullptr, &render_pass) != VK_SUCCESS)
        throw std::runtime_error("unable to create render pass");

    debugLog("    done.");

    return render_pass;
}

PTPipeline PTApplication::constructPipeline(const PTShader& shader, const VkRenderPass render_pass)
{
    debugLog("    constructing pipeline...");
    // TODO: move all this code into the pipeline class?

    // TODO: convert to param
    std::vector<VkDynamicState> dynamic_states = { };
    VkPipelineDynamicStateCreateInfo dynamic_state_create_info{ };
    dynamic_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamic_state_create_info.dynamicStateCount = static_cast<uint32_t>(dynamic_states.size());
    dynamic_state_create_info.pDynamicStates = dynamic_states.data();

    auto binding_description = getVertexBindingDescription();
    auto attribute_descriptions = getVertexAttributeDescriptions();

    VkPipelineVertexInputStateCreateInfo vertex_input_create_info{ };
    vertex_input_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertex_input_create_info.vertexBindingDescriptionCount = 1;
    vertex_input_create_info.pVertexBindingDescriptions = &binding_description;
    vertex_input_create_info.vertexAttributeDescriptionCount = static_cast<uint32_t>(attribute_descriptions.size());
    vertex_input_create_info.pVertexAttributeDescriptions = attribute_descriptions.data();

    VkPipelineDepthStencilStateCreateInfo depth_create_info{ };
    depth_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depth_create_info.depthWriteEnable = VK_TRUE;
    depth_create_info.depthTestEnable = VK_TRUE;
    depth_create_info.depthCompareOp = VK_COMPARE_OP_LESS;
    depth_create_info.depthBoundsTestEnable = VK_FALSE;
    depth_create_info.stencilTestEnable = VK_FALSE;

    // TODO: needs options to switch to line mode
    VkPipelineInputAssemblyStateCreateInfo input_assembly_create_info{ };
    input_assembly_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_assembly_create_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    input_assembly_create_info.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport{ };
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)swapchain->getExtent().width;
    viewport.height = (float)swapchain->getExtent().height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor{ };
    scissor.offset = { 0, 0 };
    scissor.extent = swapchain->getExtent();

    // TODO: this could be converted to a dynamic state
    VkPipelineViewportStateCreateInfo viewport_state_create_info{ };
    viewport_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_state_create_info.viewportCount = 1;
    viewport_state_create_info.pViewports = &viewport;
    viewport_state_create_info.scissorCount = 1;
    viewport_state_create_info.pScissors = &scissor;

    // TODO: enable line rendering, requires GPU feature
    VkPipelineRasterizationStateCreateInfo rasteriser_create_info{ };
    rasteriser_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasteriser_create_info.depthClampEnable = VK_FALSE;
    rasteriser_create_info.rasterizerDiscardEnable = VK_FALSE;
    rasteriser_create_info.polygonMode = VK_POLYGON_MODE_FILL;
    rasteriser_create_info.lineWidth = 1.0f;
    rasteriser_create_info.cullMode = VK_CULL_MODE_BACK_BIT;
    rasteriser_create_info.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasteriser_create_info.depthBiasEnable = VK_FALSE;
    rasteriser_create_info.depthBiasConstantFactor = 0.0f;
    rasteriser_create_info.depthBiasClamp = 0.0f;
    rasteriser_create_info.depthBiasSlopeFactor = 0.0f;

    VkPipelineMultisampleStateCreateInfo multisampling_create_info{ };
    multisampling_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling_create_info.sampleShadingEnable = VK_FALSE;
    multisampling_create_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampling_create_info.minSampleShading = 1.0f;
    multisampling_create_info.pSampleMask = nullptr;
    multisampling_create_info.alphaToCoverageEnable = VK_FALSE;
    multisampling_create_info.alphaToOneEnable = VK_FALSE;

    VkPipelineColorBlendAttachmentState colour_blend_attachment{ };
    colour_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colour_blend_attachment.blendEnable = VK_TRUE;
    colour_blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colour_blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colour_blend_attachment.colorBlendOp = VK_BLEND_OP_ADD;
    colour_blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colour_blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colour_blend_attachment.alphaBlendOp = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo colour_blend_create_info{ };
    colour_blend_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colour_blend_create_info.logicOpEnable = VK_FALSE;
    colour_blend_create_info.logicOp = VK_LOGIC_OP_COPY;
    colour_blend_create_info.attachmentCount = 1;
    colour_blend_create_info.pAttachments = &colour_blend_attachment;
    colour_blend_create_info.blendConstants[0] = 0.0f;
    colour_blend_create_info.blendConstants[1] = 0.0f;
    colour_blend_create_info.blendConstants[2] = 0.0f;
    colour_blend_create_info.blendConstants[3] = 0.0f;

    VkPipelineLayoutCreateInfo pipeline_layout_create_info{ };
    pipeline_layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_create_info.setLayoutCount = 1;
    // TODO: convert this to a parameter
    pipeline_layout_create_info.pSetLayouts = &demo_descriptor_set_layout;
    pipeline_layout_create_info.pushConstantRangeCount = 0;
    pipeline_layout_create_info.pPushConstantRanges = nullptr;

    cout << "        creating pipeline layout... ";

    PTPipeline pipeline;

    if (vkCreatePipelineLayout(device, &pipeline_layout_create_info, nullptr, &(pipeline.layout)) != VK_SUCCESS)
        throw runtime_error("unable to create pipeline layout");

    debugLog("done.");

    debugLog("        creating graphics pipeline... ");

    vector<VkPipelineShaderStageCreateInfo> shader_stages = shader.getStageCreateInfo();

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
    pipeline_create_info.layout = pipeline.layout;
    pipeline_create_info.renderPass = render_pass;
    pipeline_create_info.subpass = 0;
    pipeline_create_info.basePipelineHandle = VK_NULL_HANDLE;
    pipeline_create_info.basePipelineIndex = -1;


    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipeline_create_info, nullptr, &(pipeline.pipeline)) != VK_SUCCESS)
        throw std::runtime_error("unable to create to create graphics pipeline");
    
    debugLog("done.");

    debugLog("    done.");

    return pipeline;
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

void PTApplication::createVertexBuffer()
{
    // vertex buffer creation (via staging buffer)
    VkDeviceSize size = sizeof(OLVertex) * demo_mesh.vertices.size();
    PTBuffer* staging_buffer = new PTBuffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, device, physical_device);
    
    void* vertex_data = staging_buffer->map();
    memcpy(vertex_data, demo_mesh.vertices.data(), (size_t)size);
    staging_buffer->unmap();
    vertex_buffer = new PTBuffer(size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, device, physical_device);
    staging_buffer->copyTo(vertex_buffer, size);

    delete staging_buffer;
    
    // index buffer creation (via staging buffer)
    size = sizeof(uint16_t) * demo_mesh.indices.size();
    staging_buffer = new PTBuffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, device, physical_device);
    
    void* index_data = staging_buffer->map();
    memcpy(index_data, demo_mesh.indices.data(), (size_t)size);
    staging_buffer->unmap();
    index_buffer = new PTBuffer(size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, device, physical_device);
    staging_buffer->copyTo(index_buffer, size);

    delete staging_buffer;
}

void PTApplication::createUniformBuffers()
{
    VkDeviceSize transform_buffer_size = sizeof(TransformMatrices);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        PTBuffer* buf = new PTBuffer(transform_buffer_size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, device, physical_device);
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
    createImage(swapchain->getExtent().width, swapchain->getExtent().height, VK_FORMAT_D32_SFLOAT, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, depth_image, depth_image_memory);
    depth_image_view = createImageView(depth_image, VK_FORMAT_D32_SFLOAT, VK_IMAGE_ASPECT_DEPTH_BIT);
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

    vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, demo_descriptor_set_layout);
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

void PTApplication::resizeSwapchain()
{
    debugLog("resizing swapchain + framebuffer...");
    vkDeviceWaitIdle(device);

    for (auto framebuffer : framebuffers)
        vkDestroyFramebuffer(device, framebuffer, nullptr);

    delete swapchain;

    swapchain = new PTSwapchain(width, height, physical_device, device, surface);
    
    createFramebuffers(demo_render_pass);

    debugLog("done.");
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
    OLMatrix4f().getColumnMajor(transform.model_to_world);

    debugSetSceneProperty("w2c r0", to_string(current_scene->getCameraMatrix().row_0()));
    debugSetSceneProperty("w2c r1", to_string(current_scene->getCameraMatrix().row_1()));
    debugSetSceneProperty("w2c r2", to_string(current_scene->getCameraMatrix().row_2()));
    debugSetSceneProperty("w2c r3", to_string(current_scene->getCameraMatrix().row_3()));

    // cout << "blank matrix:" << endl;
    // cout << OLMatrix4f().row_0() << endl;;
    // cout << OLMatrix4f().row_1() << endl;;
    // cout << OLMatrix4f().row_2() << endl;;
    // cout << OLMatrix4f().row_3() << endl;;

    memcpy(uniform_buffers[frame_index]->getMappedMemory(), &transform, sizeof(TransformMatrices));
}

void PTApplication::loadTextureToImage(string texture_file)
{
    // TODO: load an image to a memory buffer from the file
    OLImage texture(texture_file);
    VkDeviceSize image_size = texture.getSize().x * texture.getSize().y * 4;

    PTBuffer* buf = new PTBuffer(image_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, device, physical_device);
    VkBuffer staging_buffer = buf->getBuffer();
    VkDeviceMemory staging_memory = buf->getDeviceMemory();
    
    void* data = buf->map();
    memcpy(data, texture.getData(), static_cast<size_t>(image_size));
    buf->unmap();

    VkImage texture_image;
    VkDeviceMemory texture_image_memory;

    createImage(texture.getSize().x, texture.getSize().y, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, texture_image, texture_image_memory);

    transitionImageLayout(texture_image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    copyBufferToImage(staging_buffer, texture_image, texture.getSize().x, texture.getSize().y);
    transitionImageLayout(texture_image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    delete buf;
    // TODO: when you use this, remember to delete the image and its associated device memory!
}

void PTApplication::copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height)
{
    VkCommandBuffer command_buffer = beginTransientCommands();

    VkBufferImageCopy copy_region{ };
    copy_region.bufferOffset = 0;
    copy_region.bufferRowLength = 0;
    copy_region.bufferImageHeight = 0;

    copy_region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    copy_region.imageSubresource.mipLevel = 0;
    copy_region.imageSubresource.baseArrayLayer = 0;
    copy_region.imageSubresource.layerCount = 1;

    copy_region.imageOffset = VkOffset3D{0, 0, 0};
    copy_region.imageExtent =
    {
        width,
        height,
        1
    };

    vkCmdCopyBufferToImage(command_buffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy_region);

    endTransientCommands(command_buffer);
}

void PTApplication::transitionImageLayout(VkImage image, VkFormat format, VkImageLayout old_layout, VkImageLayout new_layout)
{
    VkCommandBuffer command_buffer = beginTransientCommands();

    VkImageMemoryBarrier image_barrier{ };
    image_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    image_barrier.image = image;
    image_barrier.oldLayout = old_layout;
    image_barrier.newLayout = new_layout;
    image_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    image_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    image_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    image_barrier.subresourceRange.baseMipLevel = 0;
    image_barrier.subresourceRange.levelCount = 1;
    image_barrier.subresourceRange.baseArrayLayer = 0;
    image_barrier.subresourceRange.layerCount = 1;
    
    VkPipelineStageFlags source_stage;
    VkPipelineStageFlags destination_stage;

    if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED && new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
    {
        image_barrier.srcAccessMask = 0;
        image_barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        source_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destination_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    else if (old_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
    {
        image_barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        image_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        source_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destination_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    else
        throw invalid_argument("unsupported layout transition");

    vkCmdPipelineBarrier
    (
        command_buffer,
        source_stage, destination_stage,
        0,
        0, nullptr,
        0, nullptr,
        1, &image_barrier
    );

    endTransientCommands(command_buffer);
}

void PTApplication::createImage(uint32_t image_width, uint32_t image_height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& image_memory)
{
    VkImageCreateInfo image_create_info{ };
    image_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_create_info.imageType = VK_IMAGE_TYPE_2D;
    image_create_info.extent.width = image_width;
    image_create_info.extent.height = image_height;
    image_create_info.extent.depth = 1;
    image_create_info.mipLevels = 1;
    image_create_info.arrayLayers = 1;
    image_create_info.format = format;
    image_create_info.tiling = tiling;
    image_create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    image_create_info.usage = usage;
    image_create_info.flags = 0;
    image_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    image_create_info.samples = VK_SAMPLE_COUNT_1_BIT;

    if (vkCreateImage(device, &image_create_info, nullptr, &image) != VK_SUCCESS)
        throw runtime_error("unable to create image");
    
    VkMemoryRequirements memory_requirements{ };
    vkGetImageMemoryRequirements(device, image, &memory_requirements);

    VkMemoryAllocateInfo allocate_info{ };
    allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocate_info.allocationSize = memory_requirements.size;
    allocate_info.memoryTypeIndex = PTBuffer::findMemoryType(memory_requirements.memoryTypeBits, properties, physical_device);

    if (vkAllocateMemory(device, &allocate_info, nullptr, &image_memory) != VK_SUCCESS)
        throw runtime_error("unable to allocate image memory");

    vkBindImageMemory(device, image, image_memory, 0);
}

VkImageView PTApplication::createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspect_flags)
{
    VkImageViewCreateInfo view_create_info{ };
    view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    view_create_info.image = image;
    view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    view_create_info.format = format;
    view_create_info.subresourceRange.aspectMask = aspect_flags;
    view_create_info.subresourceRange.baseMipLevel = 0;
    view_create_info.subresourceRange.levelCount = 1;
    view_create_info.subresourceRange.baseArrayLayer = 0;
    view_create_info.subresourceRange.layerCount = 1;

    VkImageView image_view;
    if (vkCreateImageView(device, &view_create_info, nullptr, &image_view) != VK_SUCCESS)
        throw runtime_error("unable to create texture image view");

    return image_view;
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

    return score;
}
