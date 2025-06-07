#pragma once

#include <vulkan/vulkan.h>
#include <string>

#include "reference_counter.h"

class PTImage_T
{
private:
    VkDevice device = VK_NULL_HANDLE;

    VkImage image = VK_NULL_HANDLE;
    VkDeviceMemory image_memory = VK_NULL_HANDLE;
    VkExtent2D size;
    VkFormat format;
    VkImageTiling tiling;
    VkImageUsageFlags usage;
    VkImageLayout layout;

    std::string origin_path;

public:
    PTImage_T() = delete;
    PTImage_T(const PTImage_T& other) = delete;
    PTImage_T(const PTImage_T&& other) = delete;
    PTImage_T operator=(const PTImage_T& other) = delete;
    PTImage_T operator=(const PTImage_T&& other) = delete;
    ~PTImage_T();

    static inline PTCountedPointer<PTImage_T> createImage(VkExtent2D size, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties)
    { return PTCountedPointer<PTImage_T>(new PTImage_T(size, format, tiling, usage, properties)); }
    static inline PTCountedPointer<PTImage_T> createImage(std::string texture_path)
    { return PTCountedPointer<PTImage_T>(new PTImage_T(texture_path)); }

    inline VkImage getImage() const { return image; }
    inline VkDeviceMemory getImageMemory() const { return image_memory; }
    inline VkExtent2D getSize() const { return size; }
    inline VkFormat getFormat() const { return format; }
    inline VkImageTiling getTiling() const { return tiling; }
    inline VkImageUsageFlags getUsage() const { return usage; }
    inline VkImageLayout getLayout() const { return layout; }

    VkImageView createImageView(VkImageAspectFlags aspect_flags);
    void transitionImageLayout(VkImageLayout new_layout, VkCommandBuffer cmd = VK_NULL_HANDLE);
    void copyBufferToImage(VkBuffer buffer, VkCommandBuffer cmd = VK_NULL_HANDLE);

    static void transitionImageLayout(VkImage image, VkImageLayout old_layout, VkImageLayout new_layout, VkCommandBuffer cmd = VK_NULL_HANDLE);

private:
    PTImage_T(VkExtent2D _size, VkFormat _format, VkImageTiling _tiling, VkImageUsageFlags _usage, VkMemoryPropertyFlags properties);
    PTImage_T(std::string texture_path);

    void prepareImage(VkExtent2D _size, VkFormat _format, VkImageTiling _tiling, VkImageUsageFlags _usage, VkMemoryPropertyFlags properties);
};

typedef PTCountedPointer<PTImage_T> PTImage;
