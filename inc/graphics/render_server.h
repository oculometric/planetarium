#pragma once

#include <vulkan/vulkan.h>
#include <map>
#include <vector>
#include <array>
#include <thread>
#include <set>

#include "constant.h"
#include "physical_device.h"
#include "render_graph.h"

#include "resource.h"
#include "reference_counter.h"

typedef PTCountedPointer<class PTBuffer_T> PTBuffer;
typedef PTCountedPointer<class PTMesh_T> PTMesh;
typedef PTCountedPointer<class PTPipeline_T> PTPipeline;
typedef PTCountedPointer<class PTImage_T> PTImage;
typedef PTCountedPointer<class PTRenderPass_T> PTRenderPass;
typedef PTCountedPointer<class PTSwapchain_T> PTSwapchain;
typedef PTCountedPointer<class PTMaterial_T> PTMaterial;
typedef PTCountedPointer<class PTScene_T> PTScene;

class PTNode;
class PTTransform;
class PTLightNode;

struct GLFWwindow;

class PTRenderServer
{
private:
    struct DrawRequest
    {
        PTMesh mesh = nullptr;
        PTTransform* transform = nullptr;
        PTMaterial material = nullptr;
        std::array<VkDescriptorSet, MAX_FRAMES_IN_FLIGHT> descriptor_sets;
        std::array<PTBuffer, MAX_FRAMES_IN_FLIGHT> descriptor_buffers;

        static bool compare(const DrawRequest& a, const DrawRequest& b);
    };

public:
	bool debug_mode = false;

private:
	bool wants_screenshot = false;
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

    PTSwapchain swapchain = nullptr;
    std::vector<VkSemaphore> image_available_semaphores;
    std::vector<VkSemaphore> render_finished_semaphores;
    std::vector<VkFence> in_flight_fences;

    PTRGGraph render_graph = nullptr;

    std::array<PTBuffer, MAX_FRAMES_IN_FLIGHT> scene_uniform_buffers;
    
    std::multimap<PTNode*, DrawRequest> draw_queue;
    std::set<PTLightNode*> light_set;

    PTMaterial default_material = nullptr;
    PTMesh quad_mesh = nullptr;

    static constexpr char* required_device_extensions[1] =
    {
        (char*)VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

public:
    PTRenderServer(PTRenderServer& other) = delete;
    PTRenderServer(PTRenderServer&& other) = delete;
    void operator=(PTRenderServer& other) = delete;
    void operator=(PTRenderServer&& other) = delete;

	static void init(GLFWwindow* window, std::vector<const char*> glfw_extensions);
    static void deinit();
    static PTRenderServer* get();

	inline void setWantsScreenshot() { wants_screenshot = true; }
	inline void setWindowResized() { window_resized = true; }

    VkCommandBuffer beginTransientCommands();
    void endTransientCommands(VkCommandBuffer transient_command_buffer);
    
    void addDrawRequest(PTNode* owner, PTMesh mesh, PTMaterial material = nullptr, PTTransform* target_transform = nullptr);
    void removeAllDrawRequests(PTNode* owner);
    void addLight(PTLightNode* light);
    void removeLight(PTLightNode* light);

    inline PTSwapchain getSwapchain() const;
    inline PTRenderPass getRenderPass() const;
    inline PTPhysicalDevice getPhysicalDevice() const { return physical_device; }
    inline VkDevice getDevice() const { return device; }

    void beginEditLock();
    void endEditLock();
    void beginDrawLock();
    void endDrawLock();

    void update();

private:
	PTRenderServer(GLFWwindow* window, std::vector<const char*> glfw_extensions);
	~PTRenderServer();

    void initVulkan(GLFWwindow* window, std::vector<const char*> glfw_extensions);
    void deinitVulkan();

    void initVulkanInstance(std::vector<const char*>& layers, std::vector<const char*> extensions);
	void initDevice(const std::vector<const char*>& layers);
    void createCommandPoolAndBuffers();
    void createDescriptorPoolAndSets();
    void createFramebufferAndSyncResources();
	void destroyFramebufferAndSyncResources();
	VkResult createDebugUtilsMessenger(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, VkDebugUtilsMessengerEXT* pDebugMessenger);
    void destroyDebugUtilsMessenger(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger);

    void updateSceneAndTransformUniforms(uint32_t frame_index);
    void updateTextureBindings();
    void drawFrame(uint32_t frame_index);
    void generateCameraRenderStepCommands(uint32_t frame_index, VkCommandBuffer command_buffer, PTRGStepInfo step_info, std::vector<DrawRequest>& sorted_queue);
    void generatePostProcessRenderStepCommands(uint32_t frame_index, VkCommandBuffer command_buffer, PTRGStepInfo step_info, std::pair<PTMaterial, VkDescriptorSet> material);
    void generateImageLayoutTransitionCommands(VkCommandBuffer command_buffer, VkImage image, VkImageLayout old_layout, VkImageLayout new_layout, VkAccessFlags src_access, VkAccessFlags dst_access, VkPipelineStageFlags src_stage, VkPipelineStageFlags dst_stage);

    void resizeSwapchain();
    void takeScreenshot(uint32_t frame_index);

    PTPhysicalDevice selectPhysicalDevice();
    int evaluatePhysicalDevice(PTPhysicalDevice d);
};