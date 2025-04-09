#pragma once

#pragma once

#include <vulkan/vulkan.h>
#include <map>
#include <vector>
#include <array>

#include "constant.h"
#include "physical_device.h"

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

class GLFWwindow;

class PTRenderServer
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

private:
	bool wants_screenshot = false;
    bool should_stop = false;
    int width;
    int height;
    bool window_resized = false;
    int state = 0;

    VkInstance instance = VK_NULL_HANDLE;
#ifndef NDEBUG
    VkDebugUtilsMessengerEXT debug_messenger = VK_NULL_HANDLE;
#endif
    VkSurfaceKHR surface = VK_NULL_HANDLE;

    PTPhysicalDevice physical_device;
    VkDevice device = VK_NULL_HANDLE;
    std::map<PTPhysicalDevice::QueueFamily, VkQueue> queues;

    VkCommandPool command_pool = VK_NULL_HANDLE;
    std::vector<VkCommandBuffer> command_buffers;

    VkDescriptorPool descriptor_pool = VK_NULL_HANDLE;

    PTSwapchain* swapchain = nullptr;
    std::vector<VkSemaphore> image_available_semaphores;
    std::vector<VkSemaphore> render_finished_semaphores;
    std::vector<VkFence> in_flight_fences;

    std::vector<VkFramebuffer> framebuffers;
    PTImage* depth_image = nullptr;
    VkImageView depth_image_view = VK_NULL_HANDLE;
    PTImage* normal_image = nullptr;
    VkImageView normal_image_view = VK_NULL_HANDLE;

    std::array<PTBuffer*, MAX_FRAMES_IN_FLIGHT> scene_uniform_buffers;
    
    std::multimap<PTNode*, DrawRequest> draw_queue;

    PTMaterial* default_material = nullptr;
    PTRenderPass* render_pass = nullptr;

    static constexpr char* required_device_extensions[1] =
    {
        (char*)VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

public:
    PTRenderServer(PTRenderServer& other) = delete;
    PTRenderServer(PTRenderServer&& other) = delete;
    void operator=(PTRenderServer& other) = delete;
    void operator=(PTRenderServer&& other) = delete;

	static void init(GLFWwindow* window, vector<const char*> glfw_extensions);
    static void deinit();
    static PTRenderServer* get();

	inline void setWantsScreenshot() { wants_screenshot = true; }
	inline void setWindowResized() { window_resized = true; }

    VkCommandBuffer beginTransientCommands();
    void endTransientCommands(VkCommandBuffer transient_command_buffer);
    
    void addDrawRequest(PTNode* owner, PTMesh* mesh, PTMaterial* material = nullptr, PTTransform* target_transform = nullptr);
    void removeAllDrawRequests(PTNode* owner);

    void beginEditLock();
    void endEditLock();
    void beginDrawLock();
    void endDrawLock();

private:
	PTRenderServer(GLFWwindow* window, vector<const char*> glfw_extensions);
	~PTRenderServer();

    void initVulkan(GLFWwindow* window, vector<const char*> glfw_extensions);
    void mainLoop();
    void deinitVulkan();

    void initVulkanInstance(std::vector<const char*>& layers);
	void initDevice(const std::vector<const char*>& layers);
    void createCommandPoolAndBuffers();
    void createDescriptorPoolAndSets();
    void createFramebufferAndSyncResources();
	void destroyFramebufferAndSyncResources();

    void updateSceneAndTransformUniforms(uint32_t frame_index);
    void drawFrame(uint32_t frame_index);

    void resizeSwapchain();
    void takeScreenshot(uint32_t frame_index);

    PTPhysicalDevice selectPhysicalDevice();
    int evaluatePhysicalDevice(PTPhysicalDevice d);
};