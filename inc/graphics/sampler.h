#pragma once

#include <vulkan/vulkan.h>

#include "reference_counter.h"

class PTSampler_T
{
private:
    VkDevice device = VK_NULL_HANDLE;

    VkSampler sampler = VK_NULL_HANDLE;
    VkSamplerAddressMode address_mode;
    VkFilter min_filter;
    VkFilter mag_filter;
    uint32_t max_anisotropy;
    // TODO: mipmaps

public:
    PTSampler_T() = delete;
    PTSampler_T(const PTSampler_T& other) = delete;
    PTSampler_T(const PTSampler_T&& other) = delete;
    PTSampler_T operator=(const PTSampler_T& other) = delete;
    PTSampler_T operator=(const PTSampler_T&& other) = delete;
    ~PTSampler_T();

    static inline PTCountedPointer<PTSampler_T> createSampler(VkSamplerAddressMode address_mode, VkFilter min_filter, VkFilter mag_filter, uint32_t max_anisotropy)
    { return PTCountedPointer<PTSampler_T>(new PTSampler_T(address_mode, min_filter, mag_filter, max_anisotropy)); }
    
    inline VkSampler getSampler() const { return sampler; }
    inline VkSamplerAddressMode getAddressMode() const { return address_mode; }
    inline VkFilter getMinFilter() const { return min_filter; }
    inline VkFilter getMaxFilter() const { return mag_filter; }
    inline uint32_t getMaxAnisotropy() const { return max_anisotropy; }

private:
    PTSampler_T(VkSamplerAddressMode _address_mode, VkFilter _min_filter, VkFilter _mag_filter, uint32_t _max_anisotropy);

    void prepareSampler();
};

typedef PTCountedPointer<PTSampler_T> PTSampler;
