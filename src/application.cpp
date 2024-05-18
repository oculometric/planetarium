#include "application.h"

#include <stdexcept>
#include <vector>
#include <cstring>
#include <iostream>

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
    VkApplicationInfo app_info;
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName = application_name;
    app_info.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
    app_info.pEngineName = "No Engine";
    app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.apiVersion = VK_API_VERSION_1_3;
    app_info.pNext = nullptr;

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

    VkInstanceCreateInfo create_info;
    create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    create_info.pNext = nullptr;
    create_info.pApplicationInfo = &app_info;
    uint32_t glfw_extension_count = 0;
    const char** glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);
    create_info.enabledExtensionCount = glfw_extension_count;
    create_info.ppEnabledExtensionNames = glfw_extensions;
    create_info.ppEnabledLayerNames = nullptr;
    create_info.flags = 0;
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

    // pick physical device
    cout << "   selecting physical device..." << endl;
    uint32_t device_count = 0;
    vkEnumeratePhysicalDevices(instance, &device_count, nullptr);
    if (device_count == 0)
    {
        throw runtime_error("unable to find physical device");
    }
    vector<VkPhysicalDevice> devices(device_count);
    vkEnumeratePhysicalDevices(instance, &device_count, devices.data());
    cout << "       found " << device_count << " devices" << endl;
    VkPhysicalDeviceProperties properties;
    for (VkPhysicalDevice found_device : devices)
    {
        // TODO: replace this with a scoring system
        if (isSuitableDevice(found_device, properties))
        {
            device = found_device;
            break;
        }
    }

    if (device == VK_NULL_HANDLE)
        throw runtime_error("no valid physical device found");

    cout << "       selected device: \'" << properties.deviceName << "\' (" << properties.deviceID << ")" << endl;
    cout << "   done." << endl;

    // TODO: create logical device

    cout << "done." << endl;
}

void PTApplication::mainLoop()
{
    while (!glfwWindowShouldClose(window))
        glfwPollEvents();
}

void PTApplication::deinitVulkan()
{
    vkDestroySurfaceKHR(instance, surface, nullptr);
    
    vkDestroyInstance(instance, nullptr);
}

void PTApplication::deinitWindow()
{
    glfwDestroyWindow(window);

    glfwTerminate();
}

bool PTApplication::isSuitableDevice(VkPhysicalDevice d, VkPhysicalDeviceProperties& device_properties)
{
    VkPhysicalDeviceFeatures device_features;
    vkGetPhysicalDeviceFeatures(d, &device_features);
    vkGetPhysicalDeviceProperties(d, &device_properties);

    if (device_properties.deviceType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU && device_properties.deviceType != VK_PHYSICAL_DEVICE_TYPE_CPU)
    {
        cout << "       device " << device_properties.deviceName << " is not a discrete GPU/CPU" << endl;
        return false;
    }
    if (device_features.geometryShader != VK_TRUE)
    {
        cout << "       device " << device_properties.deviceName << " does not support geometry shader" << endl;
        return false;
    }
    if (device_features.fillModeNonSolid != VK_TRUE)
    {
        cout << "       device " << device_properties.deviceName << " does not support non-solid fill" << endl;
        return false;
    }

    return true;
}
