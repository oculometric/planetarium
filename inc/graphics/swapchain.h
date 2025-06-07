#pragma once

#include <vulkan/vulkan.h>
#include <vector>

#include "reference_counter.h"

class PTSwapchain_T
{
private:
    VkDevice device = VK_NULL_HANDLE;

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

public:
    PTSwapchain_T() = delete;
    PTSwapchain_T(const PTSwapchain_T& other) = delete;
    PTSwapchain_T(const PTSwapchain_T&& other) = delete;
    PTSwapchain_T operator=(const PTSwapchain_T& other) = delete;
    PTSwapchain_T operator=(const PTSwapchain_T&& other) = delete;
    ~PTSwapchain_T();

    static inline PTCountedPointer<PTSwapchain_T> createSwapchain(VkSurfaceKHR surface, int window_x, int window_y)
    { return PTCountedPointer<PTSwapchain_T>(new PTSwapchain_T(surface, window_x, window_y)); }

    inline VkSwapchainKHR getSwapchain() const { return swapchain; }
    inline VkSurfaceFormatKHR getSurfaceFormat() const { return surface_format; }
    inline VkFormat getImageFormat() const { return image_format; }
    inline uint32_t getImageCount() const { return image_count; }
    inline VkExtent2D getExtent() const { return extent; }
    inline VkImage getImage(uint32_t index) const { return images[index]; }
    inline VkImageView getImageView(uint32_t index) const { return image_views[index]; }

    void resize(VkSurfaceKHR surface, int size_x, int size_y);
   
private:
    PTSwapchain_T(VkSurfaceKHR surface, int window_x, int window_y);

    void prepareSwapchain(VkSurfaceKHR surface);
    void collectImages();
};

typedef PTCountedPointer<PTSwapchain_T> PTSwapchain;
