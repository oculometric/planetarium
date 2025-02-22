#include "swapchain.h"

#include <stdexcept>

using namespace std;

inline uint32_t clamp(uint32_t x, uint32_t mini, uint32_t maxi)
{
    float y = x > maxi ? maxi : x;
    return (y < mini) ? mini : y;
}

PTSwapchain::PTSwapchain(VkDevice device, PTPhysicalDevice& physical_device, VkSurfaceKHR surface, int window_x, int window_y)
{
    // decide on surface format
    VkSurfaceFormatKHR selected_surface_format = physical_device.getSwapchainFormats()[0];
    for (const auto& format : physical_device.getSwapchainFormats())
    {
        if (format.format == VK_FORMAT_B8G8R8A8_SRGB && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
            selected_surface_format = format;
            break;
        }
    }

    // decide on present mode
    VkPresentModeKHR selected_surface_present_mode = VK_PRESENT_MODE_FIFO_KHR;

    physical_device.refreshInfo(surface);
    VkSurfaceCapabilitiesKHR capabilities = physical_device.getSwapchainCapabilities();
    // decide on extent
    VkExtent2D selected_extent;
    if (capabilities.currentExtent.width != UINT32_MAX)
        selected_extent = capabilities.currentExtent;
    else
    {
        selected_extent.width = (uint32_t)window_x;
        selected_extent.height = (uint32_t)window_y;

        selected_extent.width = clamp(selected_extent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        selected_extent.height = clamp(selected_extent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
    }

    // decide on image count
    uint32_t selected_image_count = capabilities.minImageCount + 1;
    if (capabilities.maxImageCount > 0 && selected_image_count > capabilities.maxImageCount)
        selected_image_count = capabilities.maxImageCount;

    // create swapchain
    VkSwapchainCreateInfoKHR swap_chain_create_info{ };
    swap_chain_create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swap_chain_create_info.surface = surface;
    swap_chain_create_info.minImageCount = selected_image_count;
    swap_chain_create_info.imageFormat = selected_surface_format.format;
    swap_chain_create_info.imageColorSpace = selected_surface_format.colorSpace;
    swap_chain_create_info.imageExtent = selected_extent;
    swap_chain_create_info.imageArrayLayers = 1;
    swap_chain_create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

    // check if the queues are the same
    if (physical_device.getQueueFamily(PTQueueFamily::PRESENT) != physical_device.getQueueFamily(PTQueueFamily::GRAPHICS))
    {
        vector<uint32_t> queue_family_indices(physical_device.getAllQueueFamilies().size());
        for (pair<PTQueueFamily, uint32_t> family : physical_device.getAllQueueFamilies())
            queue_family_indices.push_back(family.second);
        queue_families = queue_family_indices;
        swap_chain_create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        swap_chain_create_info.queueFamilyIndexCount = queue_family_indices.size();
        swap_chain_create_info.pQueueFamilyIndices = queue_family_indices.data();
        sharing_mode = VK_SHARING_MODE_CONCURRENT;
    }
    else
    {
        swap_chain_create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        sharing_mode = VK_SHARING_MODE_EXCLUSIVE;
    }

    swap_chain_create_info.preTransform = capabilities.currentTransform;
    swap_chain_create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swap_chain_create_info.presentMode = selected_surface_present_mode;
    swap_chain_create_info.clipped = VK_TRUE;
    swap_chain_create_info.oldSwapchain = VK_NULL_HANDLE;

    if (vkCreateSwapchainKHR(device, &swap_chain_create_info, nullptr, &swapchain) != VK_SUCCESS)
        throw runtime_error("unable to create swap chain");

    surface_format = selected_surface_format;
    image_format = selected_surface_format.format;
    extent = selected_extent;
    image_count = selected_image_count;
    surface_present_mode = selected_surface_present_mode;
    transform = capabilities.currentTransform;

    // collect swapchain images
    vkGetSwapchainImagesKHR(device, swapchain, &selected_image_count, nullptr);
    images.resize(selected_image_count);
    vkGetSwapchainImagesKHR(device, swapchain, &selected_image_count, images.data());

    // create swapchain image views
    image_views.resize(images.size());
    for (size_t i = 0; i < images.size(); i++)
    {
        VkImageViewCreateInfo view_create_info{ };
        view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        view_create_info.image = images[i];
        view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        view_create_info.format = image_format;
        view_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        view_create_info.subresourceRange.baseMipLevel = 0;
        view_create_info.subresourceRange.levelCount = 1;
        view_create_info.subresourceRange.baseArrayLayer = 0;
        view_create_info.subresourceRange.layerCount = 1;

        VkImageView image_view;
        if (vkCreateImageView(device, &view_create_info, nullptr, &image_view) != VK_SUCCESS)
            throw runtime_error("unable to create texture image view");
        image_views[i] = image_view;
    }

    target_device = device;
}

PTSwapchain::~PTSwapchain()
{
    for (auto image_view : image_views)
        vkDestroyImageView(target_device, image_view, nullptr);

    vkDestroySwapchainKHR(target_device, swapchain, nullptr);
}

void PTSwapchain::resize(VkSurfaceKHR surface, int size_x, int size_y)
{
    for (auto image_view : image_views)
        vkDestroyImageView(target_device, image_view, nullptr);

    vkDestroySwapchainKHR(target_device, swapchain, nullptr);

    extent = VkExtent2D{ static_cast<uint32_t>(size_x), static_cast<uint32_t>(size_y) };

    // create swapchain
    VkSwapchainCreateInfoKHR swap_chain_create_info{ };
    swap_chain_create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swap_chain_create_info.surface = surface;
    swap_chain_create_info.minImageCount = image_count;
    swap_chain_create_info.imageFormat = surface_format.format;
    swap_chain_create_info.imageColorSpace = surface_format.colorSpace;
    swap_chain_create_info.imageExtent = extent;
    swap_chain_create_info.imageArrayLayers = 1;
    swap_chain_create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    if (sharing_mode == VK_SHARING_MODE_CONCURRENT)
    {
        swap_chain_create_info.queueFamilyIndexCount = queue_families.size();
        swap_chain_create_info.pQueueFamilyIndices = queue_families.data();
    }
    swap_chain_create_info.imageSharingMode = sharing_mode;
    swap_chain_create_info.preTransform = transform;
    swap_chain_create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swap_chain_create_info.presentMode = surface_present_mode;
    swap_chain_create_info.clipped = VK_TRUE;
    swap_chain_create_info.oldSwapchain = VK_NULL_HANDLE;

    if (vkCreateSwapchainKHR(target_device, &swap_chain_create_info, nullptr, &swapchain) != VK_SUCCESS)
        throw runtime_error("unable to create swap chain");
}
