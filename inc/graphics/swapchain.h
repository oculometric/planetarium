#pragma once

#include <vulkan/vulkan.h>
#include <vector>

#include "resource.h"
#include "physical_device.h"

class PTSwapchain : public PTResource
{
    friend class PTResourceManager;
private:
    VkDevice target_device = VK_NULL_HANDLE;

    VkSwapchainKHR swapchain = VK_NULL_HANDLE;
    VkSurfaceFormatKHR surface_format;
    VkFormat image_format;
    uint32_t image_count;
    VkExtent2D extent;
    std::vector<uint32_t> queue_families;
    VkPresentModeKHR surface_present_mode;
    VkSharingMode sharing_mode;
    VkSurfaceTransformFlagBitsKHR transform;
    std::vector<VkImage> images;
    std::vector<VkImageView> image_views;

    PTSwapchain(VkDevice device, PTPhysicalDevice& physical_device, VkSurfaceKHR surface, int window_x, int window_y);

    ~PTSwapchain();

public:
    PTSwapchain() = delete;
    PTSwapchain(const PTSwapchain& other) = delete;
    PTSwapchain(const PTSwapchain&& other) = delete;
    PTSwapchain operator=(const PTSwapchain& other) = delete;
    PTSwapchain operator=(const PTSwapchain&& other) = delete;

    inline VkSwapchainKHR getSwapchain() const { return swapchain; }
    inline VkSurfaceFormatKHR getSurfaceFormat() const { return surface_format; }
    inline VkFormat getImageFormat() const { return image_format; }
    inline uint32_t getImageCount() const { return image_count; }
    inline VkExtent2D getExtent() const { return extent; }
    inline VkImage getImage(uint32_t index) const { return images[index]; }
    inline VkImageView getImageView(uint32_t index) const { return image_views[index]; }

    void resize(VkSurfaceKHR surface, int size_x, int size_y);
};