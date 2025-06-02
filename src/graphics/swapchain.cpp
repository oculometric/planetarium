#include "swapchain.h"

#include <stdexcept>

using namespace std;

inline uint32_t clamp(uint32_t x, uint32_t mini, uint32_t maxi)
{
    uint32_t y = x > maxi ? maxi : x;
    return (y < mini) ? mini : y;
}

PTSwapchain::PTSwapchain(VkDevice _device, PTPhysicalDevice& physical_device, VkSurfaceKHR surface, int window_x, int window_y)
{
    // decide on surface format
    for (const auto& format : physical_device.getSwapchainFormats())
    {
        if (format.format == VK_FORMAT_B8G8R8A8_SRGB && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
            surface_format = format;
            break;
        }
    }
    image_format = surface_format.format;

    // decide on present mode

    physical_device.refreshInfo(surface);
    VkSurfaceCapabilitiesKHR capabilities = physical_device.getSwapchainCapabilities();
    // decide on extent
    if (capabilities.currentExtent.width != UINT32_MAX)
        extent = capabilities.currentExtent;
    else
    {
        extent.width = (uint32_t)window_x;
        extent.height = (uint32_t)window_y;

        extent.width = clamp(extent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        extent.height = clamp(extent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
    }

    // decide on image count
    image_count = capabilities.minImageCount + 1;
    if (capabilities.maxImageCount > 0 && image_count > capabilities.maxImageCount)
        image_count = capabilities.maxImageCount;

    // check if the queues are the same
    if (physical_device.getQueueFamily(PTPhysicalDevice::QueueFamily::PRESENT) != physical_device.getQueueFamily(PTPhysicalDevice::QueueFamily::GRAPHICS))
    {
        vector<uint32_t> queue_family_indices(physical_device.getAllQueueFamilies().size());
        for (pair<PTPhysicalDevice::QueueFamily, uint32_t> family : physical_device.getAllQueueFamilies())
            queue_family_indices.push_back(family.second);
        queue_families = queue_family_indices;
        sharing_mode = VK_SHARING_MODE_CONCURRENT;
    }
    else
    {
        sharing_mode = VK_SHARING_MODE_EXCLUSIVE;
    }

    surface_present_mode = VK_PRESENT_MODE_FIFO_KHR;
    transform = capabilities.currentTransform;
    device = _device;

    // build and collect
    createSwapchain(surface);

    collectImages();
}

PTSwapchain::~PTSwapchain()
{
    // destroy the image views and the swapchain
    for (auto image_view : image_views)
        vkDestroyImageView(device, image_view, nullptr);

    vkDestroySwapchainKHR(device, swapchain, nullptr);
}

void PTSwapchain::resize(VkSurfaceKHR surface, int size_x, int size_y)
{
    // destroy the image views and the swapchain
    for (auto image_view : image_views)
        vkDestroyImageView(device, image_view, nullptr);

    vkDestroySwapchainKHR(device, swapchain, nullptr);

    // recalculate the extents
    extent = VkExtent2D{ static_cast<uint32_t>(size_x), static_cast<uint32_t>(size_y) };

    // rebuild everything
    createSwapchain(surface);

    collectImages();
}

void PTSwapchain::createSwapchain(VkSurfaceKHR surface)
{
    // create swapchain using the stashed parameters
    VkSwapchainCreateInfoKHR swap_chain_create_info{ };
    swap_chain_create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swap_chain_create_info.surface = surface;
    swap_chain_create_info.minImageCount = image_count;
    swap_chain_create_info.imageFormat = surface_format.format;
    swap_chain_create_info.imageColorSpace = surface_format.colorSpace;
    swap_chain_create_info.imageExtent = extent;
    swap_chain_create_info.imageArrayLayers = 1;
    swap_chain_create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    if (sharing_mode == VK_SHARING_MODE_CONCURRENT)
    {
        swap_chain_create_info.queueFamilyIndexCount = static_cast<uint32_t>(queue_families.size());
        swap_chain_create_info.pQueueFamilyIndices = queue_families.data();
    }
    swap_chain_create_info.imageSharingMode = sharing_mode;
    swap_chain_create_info.preTransform = transform;
    swap_chain_create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swap_chain_create_info.presentMode = surface_present_mode;
    swap_chain_create_info.clipped = VK_TRUE;
    swap_chain_create_info.oldSwapchain = VK_NULL_HANDLE;

    if (vkCreateSwapchainKHR(device, &swap_chain_create_info, nullptr, &swapchain) != VK_SUCCESS)
        throw runtime_error("unable to create swap chain");
}

void PTSwapchain::collectImages()
{
    // collect swapchain images
    vkGetSwapchainImagesKHR(device, swapchain, &image_count, nullptr);
    images.resize(image_count);
    vkGetSwapchainImagesKHR(device, swapchain, &image_count, images.data());

    // create swapchain image views
    image_views.resize(images.size());
    for (size_t i = 0; i < images.size(); i++)
    {
        VkImageViewCreateInfo view_create_info{ };
        view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        view_create_info.image = images[i];
        view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        view_create_info.format = image_format;     // image view format must match the swapchain image format
        view_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        view_create_info.subresourceRange.baseMipLevel = 0;
        view_create_info.subresourceRange.levelCount = 1;
        view_create_info.subresourceRange.baseArrayLayer = 0;
        view_create_info.subresourceRange.layerCount = 1;

        if (vkCreateImageView(device, &view_create_info, nullptr, &(image_views[i])) != VK_SUCCESS)
            throw runtime_error("unable to create texture image view");
    }
}
