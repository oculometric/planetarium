#include "buffer.h"

#include <stdexcept>

#include "render_server.h"

using namespace std;

PTBuffer_T::PTBuffer_T(VkDeviceSize buffer_size, VkBufferUsageFlags usage_flags, VkMemoryPropertyFlags memory_flags)
{
    size = buffer_size;
    flags = memory_flags;

    device = PTRenderServer::get()->getDevice();
    
    // create the buffer itself
    VkBufferCreateInfo buffer_create_info{ };
    buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_create_info.size = size;
    buffer_create_info.usage = usage_flags;
    buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(device, &buffer_create_info, nullptr, &buffer) != VK_SUCCESS)
        throw runtime_error("unable to construct buffer");
    
    // check the memory requirements for the buffer
    VkMemoryRequirements memory_requirements;
    vkGetBufferMemoryRequirements(device, buffer, &memory_requirements);

    // allocate memory and bind it to the buffer
    VkMemoryAllocateInfo allocate_info{ };
    allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocate_info.allocationSize = memory_requirements.size;
    allocate_info.memoryTypeIndex = findMemoryType(memory_requirements.memoryTypeBits, memory_flags);

    if (vkAllocateMemory(device, &allocate_info, nullptr, &device_memory) != VK_SUCCESS)
        throw runtime_error("unable to allocate buffer memory");

    vkBindBufferMemory(device, buffer, device_memory, 0);
}

void* PTBuffer_T::map(VkMemoryMapFlags mapping_flags)
{
    // if it's already mapped, just return
    if (mapped_memory != nullptr)
        return mapped_memory;

    // otherwie, map it and return the pointer
    vkMapMemory(device, device_memory, 0, size, mapping_flags, &mapped_memory);

    return mapped_memory;
}

void PTBuffer_T::unmap()
{
    // if not mapped, do nothing
    if (mapped_memory == nullptr)
        return;

    // otherwise, unmap and unreference
    vkUnmapMemory(device, device_memory);
    mapped_memory = nullptr;
}

void PTBuffer_T::copyTo(PTCountedPointer<PTBuffer_T> destination, VkDeviceSize length, VkDeviceSize source_offset, VkDeviceSize destination_offset)
{
    // create, and execute, a copy command
    VkCommandBuffer copy_command_buffer = PTRenderServer::get()->beginTransientCommands();

    VkBufferCopy copy_command{ };
    copy_command.dstOffset = destination_offset;
    copy_command.srcOffset = source_offset;
    copy_command.size = length;

    vkCmdCopyBuffer(copy_command_buffer, buffer, destination->getBuffer(), 1, &copy_command);

    PTRenderServer::get()->endTransientCommands(copy_command_buffer);
}

uint32_t PTBuffer_T::findMemoryType(uint32_t type_bits, VkMemoryPropertyFlags properties)
{
    // figure out what type of memory fits the requirements
    VkPhysicalDeviceMemoryProperties memory_properties;
    vkGetPhysicalDeviceMemoryProperties(PTRenderServer::get()->getPhysicalDevice().getDevice(), &memory_properties);

    for (uint32_t i = 0; i < memory_properties.memoryTypeCount; i++)
        if ((type_bits & (1 << i)) && (memory_properties.memoryTypes[i].propertyFlags & properties) == properties) return i;

    throw runtime_error("unable to find suitable memory type");
}

PTBuffer_T::~PTBuffer_T()
{
    // make sure we're unmapped
    unmap();

    // destroy the buffer and free associated memory
    vkDestroyBuffer(device, buffer, nullptr);
    vkFreeMemory(device, device_memory, nullptr);
}
