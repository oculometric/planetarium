#include "application.h"

#include <stdexcept>
#include <vector>
#include <cstring>
#include <iostream>
#include <map>

#include <vulkan/vk_enum_string_helper.h>

using namespace std;

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
        const char* target_debug_layer = "VK_LAYER_KHRONOS_validation\0";
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

    // scan physical devices
    cout << "   selecting physical device..." << endl;
    uint32_t device_count = 0;
    vkEnumeratePhysicalDevices(instance, &device_count, nullptr);
    if (device_count == 0)
    {
        throw runtime_error("unable to find physical device");
    }
    vector<VkPhysicalDevice> devices(device_count);
    PTQueueFamilies queue_families;
    multimap<int, pair<VkPhysicalDevice, PTQueueFamilies>> device_scores;
    vkEnumeratePhysicalDevices(instance, &device_count, devices.data());
    cout << "       found " << device_count << " devices" << endl;
    for (VkPhysicalDevice found_device : devices)
    {
        int score = evaluatePhysicalDevice(found_device, queue_families);
        device_scores.insert(make_pair(score, make_pair(found_device, queue_families)));
    }

    // select physical device
    if (device_scores.rbegin()->first > 0)
    {
        physical_device = device_scores.rbegin()->second.first;
        queue_families = device_scores.rbegin()->second.second;
    }
    else
        throw runtime_error("no valid physical device found");

    VkPhysicalDeviceProperties properties;
    vkGetPhysicalDeviceProperties(physical_device, &properties);
    cout << "       selected device: \'" << properties.deviceName << "\' (" << properties.deviceID << ")" << " with score " << device_scores.rbegin()->first << endl;
    cout << "   done." << endl;

    // create queue indices
    VkDeviceQueueCreateInfo graphics_queue_create_info{ };
    graphics_queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    graphics_queue_create_info.queueFamilyIndex = queue_families.graphics_index;
    graphics_queue_create_info.queueCount = 1;
    float graphics_queue_priority = 1.0f;
    graphics_queue_create_info.pQueuePriorities = &graphics_queue_priority;

    // TODO: specify physical device features
    VkPhysicalDeviceFeatures features{ };

    // create logical device
    cout << "   creating logical device..." << endl;
    VkDeviceCreateInfo device_create_info{ };
    device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    device_create_info.pQueueCreateInfos = &graphics_queue_create_info;
    device_create_info.queueCreateInfoCount = 1;

    device_create_info.pEnabledFeatures = &features;

    device_create_info.enabledExtensionCount = 0;
    #ifdef NDEBUG
        device_create_info.enabledLayerCount = 0;
    #else
        device_create_info.enabledLayerCount = layers.size();
        device_create_info.ppEnabledLayerNames = layers.data();
    #endif

    if (vkCreateDevice(physical_device, &device_create_info, nullptr, &device) != VK_SUCCESS)
        throw runtime_error("unable to create device");
    cout << "   done." << endl;

    // grab queue handles
    cout << "   grabbing queue handles:" << endl;
    vkGetDeviceQueue(device, queue_families.graphics_index, 0, &queue_graphics);
    cout << "       graphics queue index: " << queue_families.graphics_index << endl;

    // create surface (target to render to)
    cout << "   creating window surface..." << endl;
    if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS)
        throw runtime_error("unable to create window surface");
    cout << "   done." << endl;

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

int PTApplication::evaluatePhysicalDevice(VkPhysicalDevice d, PTQueueFamilies& families)
{
    int score = 0;

    VkPhysicalDeviceFeatures device_features;
    VkPhysicalDeviceProperties device_properties;
    vkGetPhysicalDeviceFeatures(d, &device_features);
    vkGetPhysicalDeviceProperties(d, &device_properties);

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

    uint32_t queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(d, &queue_family_count, nullptr);
    vector<VkQueueFamilyProperties> queue_families(queue_family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(d, &queue_family_count, queue_families.data());

    families = PTQueueFamilies();
    int index = 0;
    for (const auto& queue_family : queue_families)
    {
        if (queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT)
            families.graphics_index = index;
        index++;
    }

    // TODO: check for presence of other families
    if (!areQueuesPresent(families)) return 0;

    return score;
}

bool areQueuesPresent(PTQueueFamilies& families)
{
    if (families.graphics_index == -1)
        return false;

    return true;
}
