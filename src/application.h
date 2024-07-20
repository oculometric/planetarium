#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>
#include <map>
#include <vector>
#include <array>
#include "pipeline.h"
#include "shader.h"
#include <chrono>

using namespace std;

#include "../lib/oculib/vertex.h"

const vector<OLVertex> vertices = 
{
    { { -1, -1, 0 }, { 1, 1, 1 } },
    { { 0, 1, 0 }, { 1, 0, 1 } },
    { { 1, 0, 0 }, { 0, 1, 0 } }
};

inline VkVertexInputBindingDescription getVertexBindingDescription()
{
    VkVertexInputBindingDescription description{ };
    description.binding = 0;
    description.stride = sizeof(OLVertex);
    description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    return description;
}

inline array<VkVertexInputAttributeDescription, 2> getVertexAttributeDescriptions()
{
    array<VkVertexInputAttributeDescription, 2> descriptions{ };

    descriptions[0].binding = 0;
    descriptions[0].location = 0;
    descriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    descriptions[0].offset = offsetof(OLVertex, position);

    descriptions[1].binding = 0;
    descriptions[1].location = 1;
    descriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    descriptions[1].offset = offsetof(OLVertex, colour);

    return descriptions;
}

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

    VkCommandPool command_pool;
    VkCommandBuffer command_buffer;
    
    VkSemaphore image_available_semaphore;
    VkSemaphore render_finished_semaphore;
    VkFence in_flight_fence;
    VkRenderPass demo_render_pass;
    PTPipeline demo_pipeline;
    PTShader* demo_shader;
    VkBuffer vertex_buffer;
    VkDeviceMemory vertex_buffer_memory;

    static constexpr char* required_device_extensions[1] =
    {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

    chrono::system_clock::time_point last_frame_start;
    uint32_t frame_time_running_mean_us;

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
    void createVertexBuffer();
    void createCommandPoolAndBuffer(const PTQueueFamilies& queue_families);
    void createSyncObjects();

    int evaluatePhysicalDevice(VkPhysicalDevice d, PTQueueFamilies& families, PTSwapChainDetails& swap_chain);
    uint32_t findMemoryType(uint32_t type_bits, VkMemoryPropertyFlags properties);
};