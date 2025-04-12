#pragma once

#include <vulkan/vulkan.h>

#include "resource.h"

class PTSampler : public PTResource
{
    friend class PTResourceManager;
private:
    VkDevice device = VK_NULL_HANDLE;

    VkSampler sampler = VK_NULL_HANDLE;
    VkSamplerAddressMode address_mode;
    VkFilter min_filter;
    VkFilter mag_filter;
    uint32_t max_anisotropy;
    // TODO: mipmaps

public:
    PTSampler() = delete;
    PTSampler(const PTSampler& other) = delete;
    PTSampler(const PTSampler&& other) = delete;
    PTSampler operator=(const PTSampler& other) = delete;
    PTSampler operator=(const PTSampler&& other) = delete;
    
    inline VkSampler getSampler() const { return sampler; }
    inline VkSamplerAddressMode getAddressMode() const { return address_mode; }
    inline VkFilter getMinFilter() const { return min_filter; }
    inline VkFilter getMaxFilter() const { return mag_filter; }
    inline uint32_t getMaxAnisotropy() const { return max_anisotropy; }

private:
    PTSampler(VkDevice _device, VkSamplerAddressMode _address_mode, VkFilter _min_filter, VkFilter _mag_filter, uint32_t _max_anisotropy);
    ~PTSampler();

    void createSampler();
};