#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include <map>

class PTPhysicalDevice
{
public:
	enum QueueFamily
	{
		GRAPHICS,
		PRESENT
	};

private:
    VkPhysicalDevice device = VK_NULL_HANDLE;
    VkPhysicalDeviceProperties properties = { };
    VkPhysicalDeviceFeatures features = { };
    std::map<QueueFamily, uint32_t> queue_families;
    std::vector<VkExtensionProperties> extensions;
    VkSurfaceCapabilitiesKHR swapchain_capabilities = { };
    std::vector<VkSurfaceFormatKHR> swapchain_formats;
    std::vector<VkPresentModeKHR> swapchain_present_modes;

public:
    inline PTPhysicalDevice() { }

    static std::vector<PTPhysicalDevice> enumerateDevices(VkInstance instance, VkSurfaceKHR surface);

    inline VkPhysicalDevice getDevice() const { return device; }
    inline VkPhysicalDeviceProperties getProperties() const { return properties; }
    inline VkPhysicalDeviceFeatures getFeatures() const { return features; }
    inline bool hasQueueFamily(QueueFamily family) const { return queue_families.count(family); }
    inline uint32_t getQueueFamily(QueueFamily family) const { return queue_families.at(family); }
    inline std::map<QueueFamily, uint32_t> getAllQueueFamilies() const { return queue_families; }
    inline std::vector<VkExtensionProperties> getExtensions() const { return extensions; }
    inline VkSurfaceCapabilitiesKHR getSwapchainCapabilities() const { return swapchain_capabilities; }
    inline std::vector<VkSurfaceFormatKHR> getSwapchainFormats() const { return swapchain_formats; }
    inline std::vector<VkPresentModeKHR> getSwapchainPresentModes() const { return swapchain_present_modes; }

    void refreshInfo(VkSurfaceKHR surface);
};