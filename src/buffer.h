#pragma once

#include <vulkan/vulkan.h>

#include "physical_device.h"

class PTBuffer
{
private:
    VkDevice device;

    VkBuffer buffer;
    VkDeviceSize size;
    VkMemoryPropertyFlags flags;
    VkDeviceMemory device_memory;
    void* mapped_memory = nullptr;
    
public:
    PTBuffer() = delete;
    PTBuffer(const PTBuffer& other) = delete;
    PTBuffer(const PTBuffer&& other) = delete;
    PTBuffer operator=(const PTBuffer& other) = delete;
    PTBuffer operator=(const PTBuffer&& other) = delete;

    PTBuffer(VkDevice _device, PTPhysicalDevice physical_device, VkDeviceSize buffer_size, VkBufferUsageFlags usage_flags, VkMemoryPropertyFlags memory_flags);

    inline VkDeviceSize getSize() { return size; }
    inline VkBuffer getBuffer() { return buffer; }
    inline VkMemoryPropertyFlags getFlags() { return flags; }
    inline VkDeviceMemory getDeviceMemory() { return device_memory; }
    void* map(VkMemoryMapFlags mapping_flags = 0);
    inline void* getMappedMemory() { return mapped_memory; }
    void unmap();
    void copyTo(PTBuffer* destination, VkDeviceSize length, VkDeviceSize source_offset = 0, VkDeviceSize destination_offset = 0);

    static uint32_t findMemoryType(uint32_t type_bits, VkMemoryPropertyFlags properties, PTPhysicalDevice physical_device);

    ~PTBuffer();
};