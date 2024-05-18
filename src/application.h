#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>

class PTApplication
{
private:
    unsigned int width;
    unsigned int height;

    GLFWwindow* window;
    VkInstance instance;
    VkSurfaceKHR surface;
    VkPhysicalDevice device = VK_NULL_HANDLE;

public:
    PTApplication(unsigned int _width, unsigned int _height);
    void start();

private:
    void initWindow();
    void initVulkan();
    void mainLoop();
    void deinitVulkan();
    void deinitWindow();

    bool isSuitableDevice(VkPhysicalDevice d, VkPhysicalDeviceProperties& device_properties);
};