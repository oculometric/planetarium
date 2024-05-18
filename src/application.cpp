#include "application.h"

#include <stdexcept>

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
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    window = glfwCreateWindow(width, height, "planetarium", nullptr, nullptr);
}

void PTApplication::initVulkan()
{
    VkApplicationInfo app_info;
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName = "planetarium";
    app_info.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
    app_info.pEngineName = "No Engine";
    app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.apiVersion = VK_API_VERSION_1_0;
    app_info.pNext = nullptr;

    VkInstanceCreateInfo create_info;
    create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    create_info.pNext = nullptr;
    create_info.pApplicationInfo = &app_info;
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    create_info.enabledExtensionCount = glfwExtensionCount;
    create_info.ppEnabledExtensionNames = glfwExtensions;
    create_info.ppEnabledLayerNames = nullptr;
    create_info.flags = 0;
    create_info.enabledLayerCount = 0;

    if (vkCreateInstance(&create_info, nullptr, &instance) != VK_SUCCESS)
        throw std::runtime_error("unable to create VK instance");
}

void PTApplication::mainLoop()
{
    while (!glfwWindowShouldClose(window))
        glfwPollEvents();
}

void PTApplication::deinitVulkan()
{
    vkDestroyInstance(instance, nullptr);
}

void PTApplication::deinitWindow()
{
    glfwDestroyWindow(window);

    glfwTerminate();
}
