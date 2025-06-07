#pragma once

#include <vulkan/vulkan.h>

#include "reference_counter.h"

class PTBuffer_T
{
private:
    VkDevice device;

    VkBuffer buffer;
    VkDeviceSize size;
    VkMemoryPropertyFlags flags;
    VkDeviceMemory device_memory;
    void* mapped_memory = nullptr;
    
public:
    PTBuffer_T() = delete;
    PTBuffer_T(const PTBuffer_T& other) = delete;
    PTBuffer_T(const PTBuffer_T&& other) = delete;
    PTBuffer_T operator=(const PTBuffer_T& other) = delete;
    PTBuffer_T operator=(const PTBuffer_T&& other) = delete;
    ~PTBuffer_T();

    static inline PTCountedPointer<PTBuffer_T> createBuffer(VkDeviceSize buffer_size, VkBufferUsageFlags usage_flags, VkMemoryPropertyFlags memory_flags)
    { return PTCountedPointer<PTBuffer_T>(new PTBuffer_T(buffer_size, usage_flags, memory_flags)); }

    inline VkDeviceSize getSize() { return size; }
    inline VkBuffer getBuffer() { return buffer; }
    inline VkMemoryPropertyFlags getFlags() { return flags; }
    inline VkDeviceMemory getDeviceMemory() { return device_memory; }
    void* map(VkMemoryMapFlags mapping_flags = 0);
    inline void* getMappedMemory() { return mapped_memory; }
    void unmap();
    void copyTo(PTCountedPointer<PTBuffer_T> destination, VkDeviceSize length, VkDeviceSize source_offset = 0, VkDeviceSize destination_offset = 0);

    static uint32_t findMemoryType(uint32_t type_bits, VkMemoryPropertyFlags properties);

private:
    PTBuffer_T(VkDeviceSize buffer_size, VkBufferUsageFlags usage_flags, VkMemoryPropertyFlags memory_flags);
};

typedef PTCountedPointer<PTBuffer_T> PTBuffer;
