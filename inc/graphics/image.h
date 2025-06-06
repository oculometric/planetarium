#pragma once

#include <vulkan/vulkan.h>
#include <string>

#include "resource.h"
#include "physical_device.h"

class PTImage : public PTResource
{
    friend class PTResourceManager;
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
    PTImage() = delete;
    PTImage(const PTImage& other) = delete;
    PTImage(const PTImage&& other) = delete;
    PTImage operator=(const PTImage& other) = delete;
    PTImage operator=(const PTImage&& other) = delete;

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
    PTImage(VkDevice _device, PTPhysicalDevice physical_device, VkExtent2D _size, VkFormat _format, VkImageTiling _tiling, VkImageUsageFlags _usage, VkMemoryPropertyFlags properties);
    PTImage(VkDevice _device, std::string texture_path, PTPhysicalDevice physical_device);

    ~PTImage();

    void createImage(PTPhysicalDevice physical_device, VkExtent2D _size, VkFormat _format, VkImageTiling _tiling, VkImageUsageFlags _usage, VkMemoryPropertyFlags properties);
};