#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>

struct PTQueueFamilies
{
    uint32_t graphics_index = -1;
};

bool areQueuesPresent(PTQueueFamilies& families);

class PTApplication
{
private:
    unsigned int width;
    unsigned int height;

    GLFWwindow* window;
    VkInstance instance;
    VkSurfaceKHR surface;
    VkPhysicalDevice physical_device = VK_NULL_HANDLE;
    VkDevice device;
    VkQueue queue_graphics;

public:
    PTApplication(unsigned int _width, unsigned int _height);
    void start();

private:
    void initWindow();
    void initVulkan();
    void mainLoop();
    void deinitVulkan();
    void deinitWindow();

    int evaluatePhysicalDevice(VkPhysicalDevice d, PTQueueFamilies& families);
};