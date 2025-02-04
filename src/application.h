#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>
#include <map>
#include <vector>
#include <array>
#include <chrono>

#include "input.h"
#include "defs.h"
#include "shader.h"
#include "pipeline.h"
#include "physical_device.h"
#include "swapchain.h"
#include "buffer.h"
#include "render_pass.h"
#include "image.h"

#include "../lib/oculib/mesh.h"

using namespace std;

// TODO: streamline this by separating functionality into smaller classes which handle specific parts of functionality (e.g. handling the window, handling the devices, handling resources, providing helper functions)
// TODO: create a resource management system to handle creating/destroying resources and their associated device memory

struct TransformMatrices
{
    float model_to_world[16];
    float world_to_clip[16];
};


const uint32_t MAX_FRAMES_IN_FLIGHT = 2;

class PTApplication
{
public:
    bool debug_mode = false;

private:
    int width;
    int height;
    bool window_resized = false;

    GLFWwindow* window;
    VkInstance instance;
    VkSurfaceKHR surface;

    PTPhysicalDevice physical_device;
    VkDevice device;

    PTSwapchain* swapchain;
    vector<VkFramebuffer> framebuffers;

    map<PTQueueFamily, VkQueue> queues;

    VkCommandPool command_pool;
    vector<VkCommandBuffer> command_buffers;
    VkDescriptorPool descriptor_pool;
    vector<VkDescriptorSet> descriptor_sets;
    
    vector<VkSemaphore> image_available_semaphores;
    vector<VkSemaphore> render_finished_semaphores;
    vector<VkFence> in_flight_fences;

    PTRenderPass* demo_render_pass;
    PTPipeline* demo_pipeline;
    PTPipeline* debug_pipeline;
    PTShader* demo_shader;

    PTBuffer* vertex_buffer;
    PTBuffer* index_buffer;

    vector<PTBuffer*> uniform_buffers;

    PTImage* depth_image;
    VkImageView depth_image_view;

    static constexpr char* required_device_extensions[1] =
    {
        (char*)VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

#ifdef _WIN32
    chrono::steady_clock::time_point last_frame_start;
#else
    chrono::system_clock::time_point last_frame_start;
#endif
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

    static PTApplication* get();

    VkCommandBuffer beginTransientCommands();
    void endTransientCommands(VkCommandBuffer transient_command_buffer);

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
    PTPhysicalDevice selectPhysicalDevice();
    vector<VkDeviceQueueCreateInfo> constructQueues();
    void initLogicalDevice(const vector<VkDeviceQueueCreateInfo>& queue_create_infos, const vector<const char*>& layers);
    void collectQueues();
    void createFramebuffers(const VkRenderPass render_pass);
    void createCommandPoolAndBuffers();
    void createDepthResources();
    void createVertexBuffer();
    void createUniformBuffers();
    void createDescriptorPoolAndSets();
    void createSyncObjects();

    void resizeSwapchain();
    static void windowResizeCallback(GLFWwindow* window, int new_width, int new_height);

    void updateUniformBuffers(uint32_t frame_index);
    int evaluatePhysicalDevice(PTPhysicalDevice d);
};