#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>
#include <map>
#include <vector>
#include <array>
#include <chrono>

#include "input.h"
#include "shader.h"
#include "pipeline.h"
#include "physical_device.h"
#include "swapchain.h"
#include "buffer.h"
#include "render_pass.h"
#include "image.h"
#include "mesh.h"

struct TransformMatrices
{
    float model_to_world[16];
    float world_to_clip[16];
};

const uint32_t MAX_FRAMES_IN_FLIGHT = 2;

// TODO: improve drawing system to allow objects to actually specify their shader/uniform data etc
struct PTDrawRequest
{
    PTMesh* mesh = nullptr;
    //PTShader* shader = nullptr;
    //PTPipeline* pipeline = nullptr;
    //PTRenderPass* render_pass = nullptr;
    //void* uniform_data = nullptr;
};

class PTScene;
class PTNode;

class PTApplication
{
public:
    bool debug_mode = false;

private:
    int width;
    int height;
    bool window_resized = false;

    GLFWwindow* window = nullptr;
    VkInstance instance = VK_NULL_HANDLE;
#ifndef NDEBUG
    VkDebugUtilsMessengerEXT debug_messenger = VK_NULL_HANDLE;
#endif
    VkSurfaceKHR surface = VK_NULL_HANDLE;

    PTPhysicalDevice physical_device;
    VkDevice device = VK_NULL_HANDLE;

    PTSwapchain* swapchain = nullptr;
    std::vector<VkFramebuffer> framebuffers;

    std::map<PTQueueFamily, VkQueue> queues;

    VkCommandPool command_pool = VK_NULL_HANDLE;
    std::vector<VkCommandBuffer> command_buffers;
    VkDescriptorPool descriptor_pool = VK_NULL_HANDLE;
    std::vector<VkDescriptorSet> descriptor_sets;
    
    std::vector<VkSemaphore> image_available_semaphores;
    std::vector<VkSemaphore> render_finished_semaphores;
    std::vector<VkFence> in_flight_fences;

    PTRenderPass* demo_render_pass = nullptr;
    PTPipeline* demo_pipeline = nullptr;
    PTPipeline* debug_pipeline = nullptr;
    PTShader* demo_shader = nullptr;

    std::multimap<PTNode*, PTDrawRequest> draw_queue;

    std::vector<PTBuffer*> uniform_buffers;

    PTImage* depth_image = nullptr;
    VkImageView depth_image_view = VK_NULL_HANDLE;

    static constexpr char* required_device_extensions[1] =
    {
        (char*)VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

#ifdef _WIN32
    std::chrono::steady_clock::time_point last_frame_start;
#else
    std::chrono::system_clock::time_point last_frame_start;
#endif
    uint32_t frame_time_running_mean_us;

    PTInputManager* input_manager = nullptr;
    PTScene* current_scene = nullptr;
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
    
    void addDrawRequest(PTDrawRequest request, PTNode* owner);
    void removeAllDrawRequests(PTNode* owner);

    inline float getAspectRatio() const { return (float)swapchain->getExtent().width / (float)swapchain->getExtent().height; }

private:
    void initWindow();
    void initVulkan();
    void initController();
    void mainLoop();
    void deinitController();
    void deinitVulkan();
    void deinitWindow();

    void initVulkanInstance(std::vector<const char*>& layers);
    void initSurface();
    PTPhysicalDevice selectPhysicalDevice();
    std::vector<VkDeviceQueueCreateInfo> constructQueues();
    void initLogicalDevice(const std::vector<VkDeviceQueueCreateInfo>& queue_create_infos, const std::vector<const char*>& layers);
    void collectQueues();
    void createFramebuffers(const VkRenderPass render_pass);
    void createCommandPoolAndBuffers();
    void createDepthResources();
    void createUniformBuffers();
    void createDescriptorPoolAndSets();
    void createSyncObjects();

    void drawFrame(uint32_t frame_index);
    void resizeSwapchain();
    static void windowResizeCallback(GLFWwindow* window, int new_width, int new_height);

    void updateUniformBuffers(uint32_t frame_index);
    int evaluatePhysicalDevice(PTPhysicalDevice d);
    std::vector<const char*> getRequiredExtensions();
    VkResult createDebugUtilsMessenger(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger);
    void destroyDebugUtilsMessenger(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator);
};