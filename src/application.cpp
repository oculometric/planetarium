#include "application.h"

#include <stdexcept>
#include <vector>
#include <cstring>
#include <iostream>
#include <set>
#include <algorithm>

//#include <vulkan/vk_enum_string_helper.h>

PTApplication::PTApplication(unsigned int _width, unsigned int _height)
{
    width = _width;
    height = _height;
}

void PTApplication::start()
{
    initWindow();
    initVulkan();

    mainLoop();

    deinitVulkan();
    deinitWindow();
}

void PTApplication::initWindow()
{
    cout << "initialising glfw..." << endl;
    glfwInit();

    cout << "    initialising window..." << endl;
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    window = glfwCreateWindow(width, height, "planetarium", nullptr, nullptr);

    cout << "done." << endl;
}

void PTApplication::initVulkan()
{
    cout << "initialising vulkan..." << endl;

    // initialise vulkan app instance
    vector<const char*> layers;
    initVulkanInstance(layers);

    // TODO: setup debug messenger

    // create surface (target to render to)
    initSurface();

    // scan physical devices
    PTQueueFamilies queue_families;
    PTSwapChainDetails swap_chain_info;
    initPhysicalDevice(queue_families, swap_chain_info);

    // create queue indices
    vector<VkDeviceQueueCreateInfo> queue_create_infos;
    constructQueues(queue_families, queue_create_infos);

    // create logical device
    initLogicalDevice(queue_create_infos, layers);

    // grab queue handles
    collectQueues(queue_families);

    // create swap chain
    VkSurfaceFormatKHR selected_surface_format;
    VkExtent2D selected_extent;
    uint32_t selected_image_count;
    initSwapChain(swap_chain_info, queue_families, selected_surface_format, selected_extent, selected_image_count);

    // retrieve images from the swap chain
    collectSwapChainImages(selected_surface_format, selected_extent, selected_image_count);

    // initialise a basic pipeline
    demo_shader = new PTShader(device, "demo");
    demo_render_pass = createRenderPass();
    demo_pipeline = constructPipeline(*demo_shader, demo_render_pass);

    createFramebuffers(demo_render_pass);

    createCommandPoolAndBuffer(queue_families);

    cout << "done." << endl;
}

void PTApplication::mainLoop()
{
    while (!glfwWindowShouldClose(window))
        glfwPollEvents();
}

void PTApplication::deinitVulkan()
{
    vkDestroyCommandPool(device, command_pool, nullptr);

    for (auto framebuffer : framebuffers)
        vkDestroyFramebuffer(device, framebuffer, nullptr);

    vkDestroyPipeline(device, demo_pipeline.pipeline, nullptr);
    vkDestroyPipelineLayout(device, demo_pipeline.layout, nullptr);
    delete demo_shader;
    vkDestroyRenderPass(device, demo_render_pass, nullptr);

    for (auto image_view : swap_chain_image_views)
        vkDestroyImageView(device, image_view, nullptr);

    vkDestroySwapchainKHR(device, swap_chain, nullptr);

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
    cout << "    vulkan app name: " << application_name << endl;
    VkApplicationInfo app_info{ };
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName = application_name;
    app_info.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
    app_info.pEngineName = "No Engine";
    app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.apiVersion = VK_API_VERSION_1_3;

#ifndef NDEBUG
    // get debug validation layer
    cout << "    searching for debug validation layer..." << endl;
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
    cout << "    debug validation layer found (" << target_debug_layer << ")" << endl;
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
    cout << "    extensions enabled:" << endl;
    for (size_t i = 0; i < glfw_extension_count; i++)
        cout << "       " << glfw_extensions[i] << endl;

    cout << "    creating VK instance..." << endl;
    VkResult result = vkCreateInstance(&create_info, nullptr, &instance);
    if (result != VK_SUCCESS)
    {
        cout << "    VK instance creation failed: " << /*string_VkResult(result) <<*/ endl;
        throw runtime_error("unable to create VK instance");
    }
    cout << "    done." << endl;
}

void PTApplication::initSurface()
{
    cout << "    creating window surface..." << endl;
    if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS)
        throw runtime_error("unable to create window surface");
    cout << "    done." << endl;
}

void PTApplication::initPhysicalDevice(PTQueueFamilies& queue_families, PTSwapChainDetails& swap_chain_info)
{
    cout << "    selecting physical device..." << endl;
    uint32_t device_count = 0;
    vkEnumeratePhysicalDevices(instance, &device_count, nullptr);
    if (device_count == 0)
        throw runtime_error("unable to find physical device");
    vector<VkPhysicalDevice> physical_devices(device_count);
    multimap<int, PTPhysicalDeviceDetails> device_scores;
    vkEnumeratePhysicalDevices(instance, &device_count, physical_devices.data());
    cout << "        found " << device_count << " devices" << endl;
    for (VkPhysicalDevice found_device : physical_devices)
    {
        queue_families.clear();
        int score = evaluatePhysicalDevice(found_device, queue_families, swap_chain_info);
        device_scores.insert(make_pair(score, PTPhysicalDeviceDetails{ found_device, queue_families, swap_chain_info }));
    }

    // select physical device
    if (device_scores.rbegin()->first > 0)
    {
        physical_device = device_scores.rbegin()->second.device;
        queue_families = device_scores.rbegin()->second.queue_families;
        swap_chain_info = device_scores.rbegin()->second.swap_chain;
    }
    else
        throw runtime_error("no valid physical device found");

    VkPhysicalDeviceProperties properties;
    vkGetPhysicalDeviceProperties(physical_device, &properties);
    cout << "        selected device: \'" << properties.deviceName << "\' (" << properties.deviceID << ")" << " with score " << device_scores.rbegin()->first << endl;
    cout << "    done." << endl;
}

void PTApplication::constructQueues(const PTQueueFamilies& queue_families, vector<VkDeviceQueueCreateInfo>& queue_create_infos)
{
    queue_create_infos.resize(queue_families.size());
    set<uint32_t> queue_indices;
    queue_create_infos.clear();
    float graphics_queue_priority = 1.0f;
    for (pair<PTQueueFamily, uint32_t> queue_family : queue_families)
    {
        if (queue_indices.find(queue_family.second) != queue_indices.cend())
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

void PTApplication::initLogicalDevice(const vector<VkDeviceQueueCreateInfo>& queue_create_infos, const vector<const char*>& layers)
{
    // TODO: specify physical device features
    VkPhysicalDeviceFeatures features{ };

    cout << "    creating logical device..." << endl;
    VkDeviceCreateInfo device_create_info{ };
    device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    cout << "        enabling " << queue_create_infos.size() << " device queues" << endl;
    device_create_info.pQueueCreateInfos = queue_create_infos.data();
    device_create_info.queueCreateInfoCount = queue_create_infos.size();

    device_create_info.pEnabledFeatures = &features;

    cout << "        enabling " << sizeof(required_device_extensions) / sizeof(required_device_extensions[0]) << " device extensions" << endl;
    device_create_info.enabledExtensionCount = sizeof(required_device_extensions) / sizeof(required_device_extensions[0]);
    device_create_info.ppEnabledExtensionNames = required_device_extensions;
#ifdef NDEBUG
    device_create_info.enabledLayerCount = 0;
#else
    cout << "        enabling " << layers.size() << " layers" << endl;
    device_create_info.enabledLayerCount = layers.size();
    device_create_info.ppEnabledLayerNames = layers.data();
#endif

    if (vkCreateDevice(physical_device, &device_create_info, nullptr, &device) != VK_SUCCESS)
        throw runtime_error("unable to create device");
    cout << "    done." << endl;
}

void PTApplication::collectQueues(const PTQueueFamilies& queue_families)
{
    cout << "    grabbing queue handles:" << endl;
    VkQueue queue;
    queues.clear();
    for (pair<PTQueueFamily, uint32_t> queue_family : queue_families)
    {
        vkGetDeviceQueue(device, queue_family.second, 0, &queue);
        queues.insert(make_pair(queue_family.first, queue));
        cout << "       " << queue_family.second << endl;
    }
}

void PTApplication::initSwapChain(const PTSwapChainDetails& swap_chain_info, PTQueueFamilies& queue_families, VkSurfaceFormatKHR& selected_surface_format, VkExtent2D& selected_extent, uint32_t& selected_image_count)
{
    cout << "    creating swap chain..." << endl;
    selected_surface_format = swap_chain_info.formats[0];
    for (const auto& format : swap_chain_info.formats)
    {
        if (format.format == VK_FORMAT_B8G8R8A8_SRGB && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
            selected_surface_format = format;
            break;
        }
    }
    cout << "        surface format: " << /*string_VkFormat(selected_surface_format.format) <<*/ endl;
    cout << "        colour space: " << /*string_VkColorSpaceKHR(selected_surface_format.colorSpace) <<*/ endl;

    VkPresentModeKHR selected_surface_present_mode = VK_PRESENT_MODE_FIFO_KHR;
    cout << "        present mode: " << /*string_VkPresentModeKHR(selected_surface_present_mode) <<*/ endl;

    if (swap_chain_info.capabilities.currentExtent.width != UINT32_MAX)
        selected_extent = swap_chain_info.capabilities.currentExtent;
    else
    {
        int width;
        int height;
        glfwGetFramebufferSize(window, &width, &height);

        selected_extent.width = (uint32_t)width;
        selected_extent.height = (uint32_t)height;

        selected_extent.width = clamp(selected_extent.width, swap_chain_info.capabilities.minImageExtent.width, swap_chain_info.capabilities.maxImageExtent.width);
        selected_extent.height = clamp(selected_extent.height, swap_chain_info.capabilities.minImageExtent.height, swap_chain_info.capabilities.maxImageExtent.height);
    }

    cout << "        extent: " << selected_extent.width << "," << selected_extent.height << endl;

    selected_image_count = swap_chain_info.capabilities.minImageCount + 1;
    if (swap_chain_info.capabilities.maxImageCount > 0 && selected_image_count > swap_chain_info.capabilities.maxImageCount)
        selected_image_count = swap_chain_info.capabilities.maxImageCount;

    cout << "        image count: " << selected_image_count << endl;

    VkSwapchainCreateInfoKHR swap_chain_create_info{ };
    swap_chain_create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swap_chain_create_info.surface = surface;
    swap_chain_create_info.minImageCount = selected_image_count;
    swap_chain_create_info.imageFormat = selected_surface_format.format;
    swap_chain_create_info.imageColorSpace = selected_surface_format.colorSpace;
    swap_chain_create_info.imageExtent = selected_extent;
    swap_chain_create_info.imageArrayLayers = 1;
    swap_chain_create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    if (queue_families[PTQueueFamily::PRESENT] != queue_families[PTQueueFamily::GRAPHICS])
    {
        vector<uint32_t> queue_family_indices(queue_families.size());
        for (pair<PTQueueFamily, uint32_t> family : queue_families)
            queue_family_indices.push_back(family.second);
        swap_chain_create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        swap_chain_create_info.queueFamilyIndexCount = queue_family_indices.size();
        swap_chain_create_info.pQueueFamilyIndices = queue_family_indices.data();
    }
    else
        swap_chain_create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;

    swap_chain_create_info.preTransform = swap_chain_info.capabilities.currentTransform;
    swap_chain_create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swap_chain_create_info.presentMode = selected_surface_present_mode;
    swap_chain_create_info.clipped = VK_TRUE;
    swap_chain_create_info.oldSwapchain = VK_NULL_HANDLE;

    if (vkCreateSwapchainKHR(device, &swap_chain_create_info, nullptr, &swap_chain) != VK_SUCCESS)
        throw runtime_error("unable to create swap chain");
    cout << "    done." << endl;
}

void PTApplication::collectSwapChainImages(const VkSurfaceFormatKHR& selected_surface_format, const VkExtent2D& selected_extent, uint32_t& selected_image_count)
{
    cout << "    retrieving images..." << endl;
    vkGetSwapchainImagesKHR(device, swap_chain, &selected_image_count, nullptr);
    swap_chain_images.resize(selected_image_count);
    vkGetSwapchainImagesKHR(device, swap_chain, &selected_image_count, swap_chain_images.data());

    swap_chain_image_format = selected_surface_format.format;
    swap_chain_extent = selected_extent;
    cout << "    done." << endl;

    // construct image views
    cout << "    constructing image views..." << endl;
    swap_chain_image_views.resize(swap_chain_images.size());
    for (size_t i = 0; i < swap_chain_images.size(); i++)
    {
        VkImageViewCreateInfo image_view_create_info{ };
        image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        image_view_create_info.image = swap_chain_images[i];
        image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        image_view_create_info.format = swap_chain_image_format;
        image_view_create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        image_view_create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        image_view_create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        image_view_create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        image_view_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        image_view_create_info.subresourceRange.baseMipLevel = 0;
        image_view_create_info.subresourceRange.levelCount = 1;
        image_view_create_info.subresourceRange.baseArrayLayer = 0;
        image_view_create_info.subresourceRange.layerCount = 1;

        if (vkCreateImageView(device, &image_view_create_info, nullptr, &swap_chain_image_views[i]) != VK_SUCCESS)
            throw runtime_error("unable to create image view");
    }
    cout << "    created " << swap_chain_image_views.size() << " image views." << endl;
}

VkRenderPass PTApplication::createRenderPass()
{
    cout << "    creating render pass..." << endl;

    VkAttachmentDescription colour_attachment{ };
    colour_attachment.format = swap_chain_image_format;
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

    VkSubpassDescription subpass{ };
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colour_attachment_ref;

    VkRenderPassCreateInfo render_pass_create_info{ };
    render_pass_create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_create_info.attachmentCount = 1;
    render_pass_create_info.pAttachments = &colour_attachment;
    render_pass_create_info.subpassCount = 1;
    render_pass_create_info.pSubpasses = &subpass;

    VkRenderPass render_pass;

    if (vkCreateRenderPass(device, &render_pass_create_info, nullptr, &render_pass) != VK_SUCCESS)
        throw std::runtime_error("unable to create render pass");

    cout << "    done." << endl;

    return render_pass;
}

PTPipeline PTApplication::constructPipeline(const PTShader& shader, const VkRenderPass render_pass)
{
    cout << "    constructing pipeline..." << endl;
    // TODO: move all this code into the pipeline class?

    // TODO: convert to param
    std::vector<VkDynamicState> dynamic_states = { };
    VkPipelineDynamicStateCreateInfo dynamic_state_create_info{ };
    dynamic_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamic_state_create_info.dynamicStateCount = static_cast<uint32_t>(dynamic_states.size());
    dynamic_state_create_info.pDynamicStates = dynamic_states.data();

    // TODO: passing in of vertices
    VkPipelineVertexInputStateCreateInfo vertex_input_create_info{ };
    vertex_input_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertex_input_create_info.vertexBindingDescriptionCount = 0;
    vertex_input_create_info.pVertexBindingDescriptions = nullptr;
    vertex_input_create_info.vertexAttributeDescriptionCount = 0;
    vertex_input_create_info.pVertexAttributeDescriptions = nullptr;

    // TODO: needs options to switch to line mode
    VkPipelineInputAssemblyStateCreateInfo input_assembly_create_info{ };
    input_assembly_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_assembly_create_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    input_assembly_create_info.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport{ };
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)swap_chain_extent.width;
    viewport.height = (float)swap_chain_extent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor{ };
    scissor.offset = { 0, 0 };
    scissor.extent = swap_chain_extent;

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
    pipeline_layout_create_info.setLayoutCount = 0;
    pipeline_layout_create_info.pSetLayouts = nullptr;
    pipeline_layout_create_info.pushConstantRangeCount = 0;
    pipeline_layout_create_info.pPushConstantRanges = nullptr;

    cout << "        creating pipeline layout... ";

    PTPipeline pipeline;

    if (vkCreatePipelineLayout(device, &pipeline_layout_create_info, nullptr, &(pipeline.layout)) != VK_SUCCESS)
        throw runtime_error("unable to create pipeline layout");

    cout << "done." << endl;

    cout << "        creating graphics pipeline... ";

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
    pipeline_create_info.pDepthStencilState = nullptr;
    pipeline_create_info.pColorBlendState = &colour_blend_create_info;
    pipeline_create_info.pDynamicState = &dynamic_state_create_info;
    pipeline_create_info.layout = pipeline.layout;
    pipeline_create_info.renderPass = render_pass;
    pipeline_create_info.subpass = 0;
    pipeline_create_info.basePipelineHandle = VK_NULL_HANDLE;
    pipeline_create_info.basePipelineIndex = -1;


    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipeline_create_info, nullptr, &(pipeline.pipeline)) != VK_SUCCESS)
        throw std::runtime_error("unable to create to create graphics pipeline");
    
    cout << "done." << endl;

    cout << "    done." << endl;

    return pipeline;
}

void PTApplication::createFramebuffers(const VkRenderPass render_pass)
{
    cout << "    creating framebuffers..." << endl;

    framebuffers.resize(swap_chain_image_views.size());

    for (size_t i = 0; i < swap_chain_image_views.size(); i++)
    {
        VkImageView attachments[] = { swap_chain_image_views[i] };

        VkFramebufferCreateInfo framebuffer_create_info{ };
        framebuffer_create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebuffer_create_info.renderPass = render_pass;
        framebuffer_create_info.attachmentCount = 1;
        framebuffer_create_info.pAttachments = attachments;
        framebuffer_create_info.width = swap_chain_extent.width;
        framebuffer_create_info.height = swap_chain_extent.height;
        framebuffer_create_info.layers = 1;

        if (vkCreateFramebuffer(device, &framebuffer_create_info, nullptr, &framebuffers[i]) != VK_SUCCESS)
            throw std::runtime_error("unable to create framebuffer");
    }

    cout << "        created " << framebuffers.size() << " framebuffer objects" << endl;
    cout << "    done." << endl;
}

void PTApplication::createCommandPoolAndBuffer(const PTQueueFamilies& queue_families)
{

    VkCommandPoolCreateInfo pool_create_info{ };
    pool_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    pool_create_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    pool_create_info.queueFamilyIndex = queue_families.at(PTQueueFamily::GRAPHICS);

    if (vkCreateCommandPool(device, &pool_create_info, nullptr, &command_pool))
        throw std::runtime_error("unable to create command pool");

    VkCommandBufferAllocateInfo buffer_alloc_info{ };
    buffer_alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    buffer_alloc_info.commandPool = command_pool;
    buffer_alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    buffer_alloc_info.commandBufferCount = 1;

    if (vkAllocateCommandBuffers(device, &buffer_alloc_info, &command_buffer) != VK_SUCCESS)
        throw std::runtime_error("unable to allocate command buffer");
}

int PTApplication::evaluatePhysicalDevice(VkPhysicalDevice d, PTQueueFamilies& families, PTSwapChainDetails& swap_chain)
{
    int score = 0;

    // fetch information about the device
    VkPhysicalDeviceFeatures device_features;
    VkPhysicalDeviceProperties device_properties;
    vkGetPhysicalDeviceFeatures(d, &device_features);
    vkGetPhysicalDeviceProperties(d, &device_properties);

    // award points for being a good boy
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

    // grab the list of supported queue families
    uint32_t queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(d, &queue_family_count, nullptr);
    vector<VkQueueFamilyProperties> queue_families(queue_family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(d, &queue_family_count, queue_families.data());

    // insert the families we want into the list
    families = PTQueueFamilies();
    int index = 0;
    for (const auto& queue_family : queue_families)
    {
        if (queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT)
            families.insert(make_pair(PTQueueFamily::GRAPHICS, index));
        VkBool32 present_support;
        vkGetPhysicalDeviceSurfaceSupportKHR(d, index, surface, &present_support);
        if (present_support)
            families.insert(make_pair(PTQueueFamily::PRESENT, index));
        index++;
    }

    // this device is unsuitable if it doesn't have all the queues we want
    if (!areQueuesPresent(families)) return 0;

    // check for the presence of required extensions
    uint32_t device_extension_count = 0;
    vkEnumerateDeviceExtensionProperties(d, nullptr, &device_extension_count, nullptr);
    vector<VkExtensionProperties> device_extensions(device_extension_count);
    vkEnumerateDeviceExtensionProperties(d, nullptr, &device_extension_count, device_extensions.data());

    // if any required extensions are absent, the device is unsuitable
    for (const char* extension_name : required_device_extensions)
    {
        bool found = false;
        for (VkExtensionProperties& prop : device_extensions)
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
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(d, surface, &swap_chain.capabilities);
    uint32_t swap_chain_format_count = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(d, surface, &swap_chain_format_count, nullptr);
    swap_chain.formats.clear();
    if (swap_chain_format_count > 0)
    {
        swap_chain.formats.resize(swap_chain_format_count);
        vkGetPhysicalDeviceSurfaceFormatsKHR(d, surface, &swap_chain_format_count, swap_chain.formats.data());
    }
    else
        return 0;
    uint32_t swap_chain_present_mode_count = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(d, surface, &swap_chain_present_mode_count, nullptr);
    swap_chain.present_modes.clear();
    if (swap_chain_present_mode_count > 0)
    {
        swap_chain.present_modes.resize(swap_chain_present_mode_count);
        vkGetPhysicalDeviceSurfacePresentModesKHR(d, surface, &swap_chain_present_mode_count, swap_chain.present_modes.data());
    }
    else
        return 0;

    return score;
}

bool areQueuesPresent(PTQueueFamilies& families)
{
    if (families.find(PTQueueFamily::GRAPHICS) == families.cend())
        return false;
    if (families.find(PTQueueFamily::PRESENT) == families.cend())
        return false;

    return true;
}
