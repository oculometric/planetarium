#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>
#include <map>
#include <vector>

using namespace std;

enum PTQueueFamily
{
    GRAPHICS,
    PRESENT
};

typedef map<PTQueueFamily, uint32_t> PTQueueFamilies;

struct PTSwapChainDetails
{
    VkSurfaceCapabilitiesKHR capabilities;
    vector<VkSurfaceFormatKHR> formats;
    vector<VkPresentModeKHR> present_modes;
};

struct PTPhysicalDeviceDetails
{
    VkPhysicalDevice device;
    PTQueueFamilies queue_families;
    PTSwapChainDetails swap_chain;
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

    VkSwapchainKHR swap_chain;
    VkFormat swap_chain_image_format;
    VkExtent2D swap_chain_extent;
    vector<VkImage> swap_chain_images;
    vector<VkImageView> swap_chain_image_views;

    map<PTQueueFamily, VkQueue> queues;
    
    static constexpr char* required_device_extensions[1] =
    {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

public:
    PTApplication(unsigned int _width, unsigned int _height);
    void start();

private:
    void initWindow();
    void initVulkan();
    void mainLoop();
    void deinitVulkan();
    void deinitWindow();

    int evaluatePhysicalDevice(VkPhysicalDevice d, PTQueueFamilies& families, PTSwapChainDetails& swap_chain);
};