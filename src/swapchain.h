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

    inline VkSwapchainKHR getSwapchain() { return swapchain; }
    inline VkSurfaceFormatKHR getSurfaceFormat() { return surface_format; }
    inline VkFormat getImageFormat() { return image_format; }
    inline uint32_t getImageCount() { return image_count; }
    inline VkExtent2D getExtent() { return extent; }
    inline VkImage getImage(uint32_t index) { return images[index]; }
    inline VkImageView getImageView(uint32_t index) { return image_views[index]; }
};