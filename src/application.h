#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>
#include <map>
#include <vector>
#include "pipeline.h"
#include "shader.h"

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
    vector<VkFramebuffer> framebuffers;

    map<PTQueueFamily, VkQueue> queues;
    
    VkRenderPass demo_render_pass;
    PTPipeline demo_pipeline;
    PTShader* demo_shader;

    static constexpr char* required_device_extensions[1] =
    {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

public:
    PTApplication(unsigned int _width, unsigned int _height);

    PTApplication() = delete;
    PTApplication(PTApplication& other) = delete;
    PTApplication(PTApplication&& other) = delete;
    void operator=(PTApplication& other) = delete;
    void operator=(PTApplication&& other) = delete;

    void start();

private:
    void initWindow();
    void initVulkan();
    void mainLoop();
    void deinitVulkan();
    void deinitWindow();

    void initVulkanInstance(vector<const char*>& layers);
    void initSurface();
    void initPhysicalDevice(PTQueueFamilies& queue_families, PTSwapChainDetails& swap_chain_info);
    void constructQueues(const PTQueueFamilies& queue_families, vector<VkDeviceQueueCreateInfo>& queue_create_infos);
    void initLogicalDevice(const vector<VkDeviceQueueCreateInfo>& queue_create_infos, const vector<const char*>& layers);
    void collectQueues(const PTQueueFamilies& queue_families);
    void initSwapChain(const PTSwapChainDetails& swap_chain_info, PTQueueFamilies& queue_families, VkSurfaceFormatKHR& selected_surface_format, VkExtent2D& selected_extent, uint32_t& selected_image_count);
    void collectSwapChainImages(const VkSurfaceFormatKHR& selected_surface_format, const VkExtent2D& selected_extent, uint32_t& selected_image_count);
    VkRenderPass createRenderPass();
    PTPipeline constructPipeline(const PTShader& shader, const VkRenderPass render_pass);
    void createFramebuffers(const VkRenderPass render_pass);

    int evaluatePhysicalDevice(VkPhysicalDevice d, PTQueueFamilies& families, PTSwapChainDetails& swap_chain);
};