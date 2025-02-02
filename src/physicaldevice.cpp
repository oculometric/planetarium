#include "physicaldevice.h"

std::vector<PTPhysicalDevice> PTPhysicalDevice::enumerateDevices(VkInstance instance, VkSurfaceKHR surface)
{
    uint32_t device_count = 0;
    std::vector<PTPhysicalDevice> devices;
    vkEnumeratePhysicalDevices(instance, &device_count, nullptr);
    if (device_count == 0)
        return devices;

    std::vector<VkPhysicalDevice> physical_devices(device_count);
    vkEnumeratePhysicalDevices(instance, &device_count, physical_devices.data());
    for (VkPhysicalDevice found_device : physical_devices)
    {
        PTPhysicalDevice dev;
        dev.device = found_device;
        dev.refreshInfo(surface);
        devices.push_back(dev);
    }

    return devices;
}

void PTPhysicalDevice::refreshInfo(VkSurfaceKHR surface)
{
    if (device == nullptr) return;

    // fetch properties and features
    vkGetPhysicalDeviceProperties(device, &properties);
    vkGetPhysicalDeviceFeatures(device, &features);

    // grab the list of available queue families, and make a map of what index each one occurs at
    uint32_t queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, nullptr);
    queue_families.clear();
    std::vector<VkQueueFamilyProperties> queue_family_props(queue_family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, queue_family_props.data());
    int index = 0;
    for (const auto& queue_family : queue_family_props)
    {
        if (queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT)
            queue_families.insert(std::make_pair(PTQueueFamily::GRAPHICS, index));
        VkBool32 present_support;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, index, surface, &present_support);
        if (present_support)
            queue_families.insert(std::make_pair(PTQueueFamily::PRESENT, index));
        index++;
    }

    // enumerate extensions
    uint32_t device_extension_count = 0;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &device_extension_count, nullptr);
    extensions.resize(device_extension_count);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &device_extension_count, extensions.data());

    // query swapchain support
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &swapchain_capabilities);
    uint32_t swapchain_format_count = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &swapchain_format_count, nullptr);
    swapchain_formats.clear();
    if (swapchain_format_count > 0)
    {
        swapchain_formats.resize(swapchain_format_count);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &swapchain_format_count, swapchain_formats.data());
    }

    uint32_t swapchain_present_mode_count = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &swapchain_present_mode_count, nullptr);
    swapchain_present_modes.clear();
    if (swapchain_present_mode_count > 0)
    {
        swapchain_present_modes.resize(swapchain_present_mode_count);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &swapchain_present_mode_count, swapchain_present_modes.data());
    }
}
