#include "image.h"

#include <stdexcept>
#include <cstring>

#include "buffer.h"
#include "application.h"
#include "resource_manager.h"

using namespace std;

PTImage::PTImage(VkDevice _device, PTPhysicalDevice physical_device, VkExtent2D _size, VkFormat _format, VkImageTiling _tiling, VkImageUsageFlags _usage, VkMemoryPropertyFlags properties)
{
    device = _device;
    
    createImage(physical_device, _size, _format, _tiling, _usage, properties);
}

PTImage::PTImage(VkDevice _device, PTPhysicalDevice physical_device, string texture_file)
{
    device = _device;
    // TODO: load an image to a memory buffer from the file
    //OLImage texture(texture_file);
    VkDeviceSize image_size = 0;//texture.getSize().x * texture.getSize().y * 4;

    PTBuffer* staging_buffer = PTResourceManager::get()->createBuffer(image_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    //void* data = staging_buffer->map();
    //memcpy(data, texture.getData(), static_cast<size_t>(image_size));
    //staging_buffer->unmap();

    createImage(physical_device, VkExtent2D{ /*texture.getSize().x, texture.getSize().y*/ 0, 0 }, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    transitionImageLayout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    copyBufferToImage(staging_buffer->getBuffer());
    transitionImageLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    PTResourceManager::get()->releaseResource(staging_buffer);
}

VkImageView PTImage::createImageView(VkImageAspectFlags aspect_flags)
{
    VkImageViewCreateInfo view_create_info{ };
    view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    view_create_info.image = image;
    view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    view_create_info.format = format;
    view_create_info.subresourceRange.aspectMask = aspect_flags;
    view_create_info.subresourceRange.baseMipLevel = 0;
    view_create_info.subresourceRange.levelCount = 1;
    view_create_info.subresourceRange.baseArrayLayer = 0;
    view_create_info.subresourceRange.layerCount = 1;

    VkImageView image_view;
    if (vkCreateImageView(device, &view_create_info, nullptr, &image_view) != VK_SUCCESS)
        throw runtime_error("unable to create texture image view");

    return image_view;
}

void PTImage::transitionImageLayout(VkImageLayout new_layout)
{
    if (layout == new_layout) return;
    
    VkCommandBuffer command_buffer = PTApplication::get()->beginTransientCommands();

    VkImageMemoryBarrier image_barrier{ };
    image_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    image_barrier.image = image;
    image_barrier.oldLayout = layout;
    image_barrier.newLayout = new_layout;
    image_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    image_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    image_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    image_barrier.subresourceRange.baseMipLevel = 0;
    image_barrier.subresourceRange.levelCount = 1;
    image_barrier.subresourceRange.baseArrayLayer = 0;
    image_barrier.subresourceRange.layerCount = 1;
    
    VkPipelineStageFlags source_stage;
    VkPipelineStageFlags destination_stage;

    if (layout == VK_IMAGE_LAYOUT_UNDEFINED && new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
    {
        image_barrier.srcAccessMask = 0;
        image_barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        source_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destination_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    else if (layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
    {
        image_barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        image_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        source_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destination_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    else
        throw invalid_argument("unsupported layout transition");

    vkCmdPipelineBarrier
    (
        command_buffer,
        source_stage, destination_stage,
        0,
        0, nullptr,
        0, nullptr,
        1, &image_barrier
    );

    PTApplication::get()->endTransientCommands(command_buffer);

    layout = new_layout;
}

void PTImage::copyBufferToImage(VkBuffer buffer)
{
    VkCommandBuffer command_buffer = PTApplication::get()->beginTransientCommands();

    VkBufferImageCopy copy_region{ };
    copy_region.bufferOffset = 0;
    copy_region.bufferRowLength = 0;
    copy_region.bufferImageHeight = 0;

    copy_region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    copy_region.imageSubresource.mipLevel = 0;
    copy_region.imageSubresource.baseArrayLayer = 0;
    copy_region.imageSubresource.layerCount = 1;

    copy_region.imageOffset = VkOffset3D{0, 0, 0};
    copy_region.imageExtent =
    {
        size.width,
        size.height,
        1
    };

    vkCmdCopyBufferToImage(command_buffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy_region);

    PTApplication::get()->endTransientCommands(command_buffer);
}

PTImage::~PTImage()
{
    vkDestroyImage(device, image, nullptr);
    vkFreeMemory(device, image_memory, nullptr);
}

void PTImage::createImage(PTPhysicalDevice physical_device, VkExtent2D _size, VkFormat _format, VkImageTiling _tiling, VkImageUsageFlags _usage, VkMemoryPropertyFlags properties)
{
    size = _size;
    format = _format;
    tiling = _tiling;
    usage = _usage;
    layout = VK_IMAGE_LAYOUT_UNDEFINED;

    // TODO: make image arrays a thing? (including exposing control on createImageView)
    VkImageCreateInfo image_create_info{ };
    image_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_create_info.imageType = VK_IMAGE_TYPE_2D;
    image_create_info.extent.width = size.width;
    image_create_info.extent.height = size.height;
    image_create_info.extent.depth = 1;
    image_create_info.mipLevels = 1;
    image_create_info.arrayLayers = 1;
    image_create_info.format = format;
    image_create_info.tiling = tiling;
    image_create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    image_create_info.usage = usage;
    image_create_info.flags = 0;
    image_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    image_create_info.samples = VK_SAMPLE_COUNT_1_BIT;

    if (vkCreateImage(device, &image_create_info, nullptr, &image) != VK_SUCCESS)
        throw runtime_error("unable to create image");
    
    VkMemoryRequirements memory_requirements{ };
    vkGetImageMemoryRequirements(device, image, &memory_requirements);

    VkMemoryAllocateInfo allocate_info{ };
    allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocate_info.allocationSize = memory_requirements.size;
    allocate_info.memoryTypeIndex = PTBuffer::findMemoryType(memory_requirements.memoryTypeBits, properties, physical_device);

    if (vkAllocateMemory(device, &allocate_info, nullptr, &image_memory) != VK_SUCCESS)
        throw runtime_error("unable to allocate image memory");

    vkBindImageMemory(device, image, image_memory, 0);
}
