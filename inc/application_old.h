#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>
#include <map>
#include <vector>
#include <array>
#include <chrono>

#include "constant.h"
#include "graphics/physical_device.h"

class PTScene;
class PTNode;
class PTSwapchain;
class PTShader;
class PTRenderPass;
class PTPipeline;
class PTBuffer;
class PTImage;
class PTMesh;
class PTTransform;
class PTMaterial;

#ifdef _WIN32
typedef std::chrono::steady_clock clocktype;
#else
typedef std::chrono::system_clock clocktype;
#endif

class PTApplication
{
private:
    struct DrawRequest
    {
        PTMesh* mesh = nullptr;
        PTTransform* transform = nullptr;
        PTMaterial* material = nullptr;
        std::array<VkDescriptorSet, MAX_FRAMES_IN_FLIGHT> descriptor_sets;
        std::array<PTBuffer*, MAX_FRAMES_IN_FLIGHT> descriptor_buffers;

        static bool compare(const DrawRequest& a, const DrawRequest& b);
    };

public:
    bool debug_mode = false;
    bool wants_screenshot = false;
    bool should_stop = false;

private:
    int width;
    int height;
    bool window_resized = false;
    int state = 0;

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
    std::array<PTBuffer*, MAX_FRAMES_IN_FLIGHT> scene_uniform_buffers;

    std::map<PTQueueFamily, VkQueue> queues;

    VkCommandPool command_pool = VK_NULL_HANDLE;
    std::vector<VkCommandBuffer> command_buffers;

    VkDescriptorPool descriptor_pool = VK_NULL_HANDLE;
    
    std::vector<VkSemaphore> image_available_semaphores;
    std::vector<VkSemaphore> render_finished_semaphores;
    std::vector<VkFence> in_flight_fences;

    PTMaterial* default_material = nullptr;
    PTRenderPass* render_pass = nullptr;

    std::multimap<PTNode*, DrawRequest> draw_queue;

    PTImage* depth_image = nullptr;
    VkImageView depth_image_view = VK_NULL_HANDLE;

    PTImage* normal_image = nullptr;
    VkImageView normal_image_view = VK_NULL_HANDLE;

    static constexpr char* required_device_extensions[1] =
    {
        (char*)VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

    clocktype::time_point last_frame_start;
    clocktype::time_point program_start;
    uint32_t frame_time_running_mean_us;

    PTScene* current_scene = nullptr;
public:
    PTApplication(unsigned int _width, unsigned int _height);

    PTApplication() = delete;
    PTApplication(PTApplication& other) = delete;
    PTApplication(PTApplication&& other) = delete;
    void operator=(PTApplication& other) = delete;
    void operator=(PTApplication&& other) = delete;

    void start();

    static PTApplication* get();

    VkCommandBuffer beginTransientCommands();
    void endTransientCommands(VkCommandBuffer transient_command_buffer);
    
    void addDrawRequest(PTNode* owner, PTMesh* mesh, PTMaterial* material = nullptr, PTTransform* target_transform = nullptr);
    void removeAllDrawRequests(PTNode* owner);

    float getAspectRatio() const;

    void beginEditLock();
    void endEditLock();
    void beginDrawLock();
    void endDrawLock();

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
    void createFramebuffers();
    void createCommandPoolAndBuffers();
    void createDepthResources();
    void createDescriptorPoolAndSets();
    void createSyncObjects();

    void drawFrame(uint32_t frame_index);
    void resizeSwapchain();
    static void windowResizeCallback(GLFWwindow* window, int new_width, int new_height);
    void takeScreenshot(uint32_t frame_index);

    void updateUniformBuffers(uint32_t frame_index);
    int evaluatePhysicalDevice(PTPhysicalDevice d);
    std::vector<const char*> getRequiredExtensions();
    VkResult createDebugUtilsMessenger(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger);
    void destroyDebugUtilsMessenger(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator);
};