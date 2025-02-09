#include "buffer.h"

#include <stdexcept>

#include "application.h"

using namespace std;

PTBuffer::PTBuffer(VkDevice _device, PTPhysicalDevice physical_device, VkDeviceSize buffer_size, VkBufferUsageFlags usage_flags, VkMemoryPropertyFlags memory_flags)
{
    size = buffer_size;
    flags = memory_flags;

    device = _device;
    
    VkBufferCreateInfo buffer_create_info{ };
    buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_create_info.size = size;
    buffer_create_info.usage = usage_flags;
    buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(device, &buffer_create_info, nullptr, &buffer) != VK_SUCCESS)
        throw runtime_error("unable to construct buffer");
    
    VkMemoryRequirements memory_requirements;
    vkGetBufferMemoryRequirements(device, buffer, &memory_requirements);

    VkMemoryAllocateInfo allocate_info{ };
    allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocate_info.allocationSize = memory_requirements.size;
    allocate_info.memoryTypeIndex = findMemoryType(memory_requirements.memoryTypeBits, memory_flags, physical_device);

    if (vkAllocateMemory(device, &allocate_info, nullptr, &device_memory) != VK_SUCCESS)
        throw runtime_error("unable to allocate buffer memory");

    vkBindBufferMemory(device, buffer, device_memory, 0);
}

void* PTBuffer::map(VkMemoryMapFlags mapping_flags)
{
    if (mapped_memory != nullptr)
        return mapped_memory;

    vkMapMemory(device, device_memory, 0, size, mapping_flags, &mapped_memory);

    return mapped_memory;
}

void PTBuffer::unmap()
{
    if (mapped_memory == nullptr)
        return;

    vkUnmapMemory(device, device_memory);
    mapped_memory = nullptr;
}

void PTBuffer::copyTo(PTBuffer* destination, VkDeviceSize length, VkDeviceSize source_offset, VkDeviceSize destination_offset)
{
    VkCommandBuffer copy_command_buffer = PTApplication::get()->beginTransientCommands();

    VkBufferCopy copy_command{ };
    copy_command.dstOffset = destination_offset;
    copy_command.srcOffset = source_offset;
    copy_command.size = length;

    vkCmdCopyBuffer(copy_command_buffer, buffer, destination->getBuffer(), 1, &copy_command);

    PTApplication::get()->endTransientCommands(copy_command_buffer);
}

uint32_t PTBuffer::findMemoryType(uint32_t type_bits, VkMemoryPropertyFlags properties, PTPhysicalDevice physical_device)
{
    VkPhysicalDeviceMemoryProperties memory_properties;
    vkGetPhysicalDeviceMemoryProperties(physical_device.getDevice(), &memory_properties);

    for (uint32_t i = 0; i < memory_properties.memoryTypeCount; i++)
        if ((type_bits & (1 << i)) && (memory_properties.memoryTypes[i].propertyFlags & properties) == properties) return i;

    throw runtime_error("unable to find suitable memory type");
}

PTBuffer::~PTBuffer()
{
    if (mapped_memory != nullptr)
        unmap();

    vkDestroyBuffer(device, buffer, nullptr);
    vkFreeMemory(device, device_memory, nullptr);
}
