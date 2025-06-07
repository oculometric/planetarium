#include "sampler.h"

#include <stdexcept>

#include "render_server.h"

using namespace std;

PTSampler_T::PTSampler_T(VkSamplerAddressMode _address_mode, VkFilter _min_filter, VkFilter _mag_filter, uint32_t _max_anisotropy)
{
    device = PTRenderServer::get()->getDevice();
    address_mode = _address_mode;
    min_filter = _min_filter;
    mag_filter = _mag_filter;
    max_anisotropy = _max_anisotropy;

    prepareSampler();
}

PTSampler_T::~PTSampler_T()
{
    vkDestroySampler(device, sampler, nullptr);
}

void PTSampler_T::prepareSampler()
{
    VkSamplerCreateInfo sampler_info{ };
    sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sampler_info.minFilter = min_filter;
    sampler_info.magFilter = mag_filter;
    sampler_info.addressModeU = address_mode;
    sampler_info.addressModeV = address_mode;
    sampler_info.addressModeW = address_mode;
    sampler_info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    sampler_info.anisotropyEnable = max_anisotropy > 0;
    sampler_info.maxAnisotropy = static_cast<float>(max_anisotropy);
    sampler_info.unnormalizedCoordinates = VK_FALSE;
    sampler_info.compareEnable = VK_FALSE;
    sampler_info.compareOp = VK_COMPARE_OP_ALWAYS;
    sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    sampler_info.mipLodBias = 0.0f;
    sampler_info.minLod = 0.0f;
    sampler_info.maxLod = 0.0f;

    if (vkCreateSampler(device, &sampler_info, nullptr, &sampler) != VK_SUCCESS)
        throw runtime_error("unable to create texture sampler");
}
