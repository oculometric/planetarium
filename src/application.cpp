#include "application.h"

#include <stdexcept>
#include <vector>
#include <cstring>
#include <iostream>
#include <set>

#include <vulkan/vk_enum_string_helper.h>

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

    cout << "   initialising window..." << endl;
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    window = glfwCreateWindow(width, height, "planetarium", nullptr, nullptr);
    cout << "done." << endl;
}

void PTApplication::initVulkan()
{
    // initialise vulkan app instance
    cout << "initialising vulkan..." << endl;
    const char* application_name = "planetarium";
    cout << "   vulkan app name: " << application_name << endl;
    VkApplicationInfo app_info{ };
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName = application_name;
    app_info.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
    app_info.pEngineName = "No Engine";
    app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.apiVersion = VK_API_VERSION_1_3;

    #ifndef NDEBUG
        // get debug validation layer
        cout << "   searching for debug validation layer..." << endl;
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
        cout << "   debug validation layer found (" << target_debug_layer << ")" << endl;
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
        vector<const char*> layers;
        layers.push_back(target_debug_layer);
        create_info.enabledLayerCount = layers.size();
        create_info.ppEnabledLayerNames = layers.data();
    #endif
    cout << "   extensions enabled:" << endl;
    for (size_t i = 0; i < glfw_extension_count; i++)
        cout << "       " << glfw_extensions[i] << endl;

    cout << "   creating VK instance..." << endl;
    VkResult result = vkCreateInstance(&create_info, nullptr, &instance);
    if (result != VK_SUCCESS)
    {
        cout << "   VK instance creation failed: " << string_VkResult(result) << endl;
        throw runtime_error("unable to create VK instance");
    }
    cout << "   done." << endl;
    
    // TODO: setup debug messenger

    // create surface (target to render to)
    cout << "   creating window surface..." << endl;
    if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS)
        throw runtime_error("unable to create window surface");
    cout << "   done." << endl;

    // scan physical devices
    cout << "   selecting physical device..." << endl;
    uint32_t device_count = 0;
    vkEnumeratePhysicalDevices(instance, &device_count, nullptr);
    if (device_count == 0)
        throw runtime_error("unable to find physical device");
    vector<VkPhysicalDevice> physical_devices(device_count);
    PTQueueFamilies queue_families;
    PTSwapChainDetails swap_chain;
    multimap<int, PTPhysicalDeviceDetails> device_scores;
    vkEnumeratePhysicalDevices(instance, &device_count, physical_devices.data());
    cout << "       found " << device_count << " devices" << endl;
    for (VkPhysicalDevice found_device : physical_devices)
    {
        queue_families.clear();
        int score = evaluatePhysicalDevice(found_device, queue_families, swap_chain);
        device_scores.insert(make_pair(score, PTPhysicalDeviceDetails{ found_device, queue_families, swap_chain }));
    }

    // select physical device
    if (device_scores.rbegin()->first > 0)
    {
        physical_device = device_scores.rbegin()->second.device;
        queue_families = device_scores.rbegin()->second.queue_families;
        swap_chain = device_scores.rbegin()->second.swap_chain;
    }
    else
        throw runtime_error("no valid physical device found");

    VkPhysicalDeviceProperties properties;
    vkGetPhysicalDeviceProperties(physical_device, &properties);
    cout << "       selected device: \'" << properties.deviceName << "\' (" << properties.deviceID << ")" << " with score " << device_scores.rbegin()->first << endl;
    cout << "   done." << endl;

    // create queue indices
    vector<VkDeviceQueueCreateInfo> queue_create_infos(queue_families.size());
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

    // TODO: specify physical device features
    VkPhysicalDeviceFeatures features{ };

    // create logical device
    cout << "   creating logical device..." << endl;
    VkDeviceCreateInfo device_create_info{ };
    device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    cout << "       enabling " << queue_create_infos.size() << " device queues" << endl;
    device_create_info.pQueueCreateInfos = queue_create_infos.data();
    device_create_info.queueCreateInfoCount = queue_create_infos.size();

    device_create_info.pEnabledFeatures = &features;

    cout << "       enabling " << sizeof(required_device_extensions)/sizeof(required_device_extensions[0]) << " device extensions" << endl;
    device_create_info.enabledExtensionCount = sizeof(required_device_extensions)/sizeof(required_device_extensions[0]);
    device_create_info.ppEnabledExtensionNames = required_device_extensions;
    #ifdef NDEBUG
        device_create_info.enabledLayerCount = 0;
    #else
        cout << "       enabling " << layers.size() << " layers" << endl;
        device_create_info.enabledLayerCount = layers.size();
        device_create_info.ppEnabledLayerNames = layers.data();
    #endif

    if (vkCreateDevice(physical_device, &device_create_info, nullptr, &device) != VK_SUCCESS)
        throw runtime_error("unable to create device");
    cout << "   done." << endl;

    // grab queue handles
    cout << "   grabbing queue handles:" << endl;
    VkQueue queue;
    for (pair<PTQueueFamily, uint32_t> queue_family : queue_families)
    {
        vkGetDeviceQueue(device, queue_family.second, 0, &queue);
        queues.insert(make_pair(queue_family.first, queue));
        cout << "       " << queue_family.second << endl;
    }

    cout << "done." << endl;
}

void PTApplication::mainLoop()
{
    while (!glfwWindowShouldClose(window))
        glfwPollEvents();
}

void PTApplication::deinitVulkan()
{
    vkDestroyDevice(device, nullptr);

    vkDestroySurfaceKHR(instance, surface, nullptr);
    
    vkDestroyInstance(instance, nullptr);
}

void PTApplication::deinitWindow()
{
    glfwDestroyWindow(window);

    glfwTerminate();
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
