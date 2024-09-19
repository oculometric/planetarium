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
#include "input.h"
#include "defs.h"
#include "../lib/oculib/mesh.h"

using namespace std;

// TODO: streamline this by separating functionality into smaller classes which handle specific parts of functionality (e.g. handling the window, handling the devices, handling resources, providing helper functions)
// TODO: create a resource management system to handle creating/destroying resources and their associated device memory

struct TransformMatrices
{
    float model_to_world[16];
    float world_to_clip[16];
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

const uint32_t MAX_FRAMES_IN_FLIGHT = 2;

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
    vector<VkCommandBuffer> command_buffers;
    VkDescriptorPool descriptor_pool;
    vector<VkDescriptorSet> descriptor_sets;
    
    vector<VkSemaphore> image_available_semaphores;
    vector<VkSemaphore> render_finished_semaphores;
    vector<VkFence> in_flight_fences;

    VkRenderPass demo_render_pass;
    VkDescriptorSetLayout demo_descriptor_set_layout;
    PTPipeline demo_pipeline;
    PTShader* demo_shader;

    VkBuffer vertex_buffer;
    VkDeviceMemory vertex_buffer_memory;
    VkBuffer index_buffer;
    VkDeviceMemory index_buffer_memory;

    vector<VkBuffer> uniform_buffers;
    vector<VkDeviceMemory> uniform_buffers_memory;
    vector<void*> uniform_buffers_mapped;

    VkImage depth_image;
    VkDeviceMemory depth_image_memory;
    VkImageView depth_image_view;

    static constexpr char* required_device_extensions[1] =
    {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

    chrono::system_clock::time_point last_frame_start;
    uint32_t frame_time_running_mean_us;

    PTInputManager* input_manager;
    PTScene* current_scene;
    OLMesh demo_mesh;
public:
    PTApplication(unsigned int _width, unsigned int _height);

    PTApplication() = delete;
    PTApplication(PTApplication& other) = delete;
    PTApplication(PTApplication&& other) = delete;
    void operator=(PTApplication& other) = delete;
    void operator=(PTApplication&& other) = delete;

    void start();

    PTInputManager* getInputManager();

private:
    void initWindow();
    void initVulkan();
    void initController();
    void mainLoop();
    void deinitController();
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
    void createDescriptorSetLayout();
    VkRenderPass createRenderPass();
    PTPipeline constructPipeline(const PTShader& shader, const VkRenderPass render_pass);
    void createFramebuffers(const VkRenderPass render_pass);
    void createCommandPoolAndBuffers(const PTQueueFamilies& queue_families);
    void createDepthResources();
    void createVertexBuffer();
    void createUniformBuffers();
    void createDescriptorPoolAndSets();
    void createSyncObjects();

    VkCommandBuffer beginTransientCommands();
    void endTransientCommands(VkCommandBuffer transient_command_buffer);
    void updateUniformBuffers(uint32_t frame_index);
    void loadTextureToImage(string texture_file);
    void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout old_layout, VkImageLayout new_layout);
    void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
    void createImage(uint32_t image_width, uint32_t image_height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& image_memory);
    VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspect_flags);
    void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage_flags, VkMemoryPropertyFlags memory_flags, VkBuffer& buffer, VkDeviceMemory& buffer_memory);
    void copyBuffer(VkBuffer source, VkBuffer destination, VkDeviceSize size);
    int evaluatePhysicalDevice(VkPhysicalDevice d, PTQueueFamilies& families, PTSwapChainDetails& swap_chain);
    uint32_t findMemoryType(uint32_t type_bits, VkMemoryPropertyFlags properties);
};