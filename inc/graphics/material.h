#pragma once

#include <vulkan/vulkan.h>
#include <string>
#include <map>

#include "constant.h"
#include "resource.h"
#include "shader.h"
#include "ptmath.h"

class PTImage;
class PTBuffer;
class PTRenderPass;
class PTPipeline;
class PTSwapchain;
class PTSampler;

class PTMaterial : public PTResource
{
public:
    friend class PTResourceManager;
private:
    VkDevice device = VK_NULL_HANDLE;
    PTShader* shader = nullptr;
    PTRenderPass* render_pass = nullptr;
    PTPipeline* pipeline = nullptr;
    VkDescriptorPool descriptor_pool = VK_NULL_HANDLE;
    std::map<uint16_t, PTBuffer*> uniform_buffers;
    std::map<uint16_t, std::pair<PTImage*, std::pair<VkImageView, PTSampler*>>> textures;
    bool needs_texture_update = false;

    int priority = 0;

    std::string origin_path;

public:
    PTMaterial(const PTMaterial& other) = delete;
    PTMaterial(const PTMaterial&& other) = delete;
    PTMaterial operator=(const PTMaterial& other) = delete;
    PTMaterial operator=(const PTMaterial&& other) = delete;

    inline PTShader* getShader() const { return shader; }
    inline PTRenderPass* getRenderPass() const { return render_pass; }
    inline PTPipeline* getPipeline() const { return pipeline; }
    inline PTBuffer* getDescriptorBuffer(uint16_t binding) { return uniform_buffers[binding]; }
    void applySetWrites(VkDescriptorSet descriptor_set);

    inline int getPriority() const { return priority; }
    inline void setPriority(int p) { priority = p; }

    void setTexture(uint16_t bind_point, PTImage* texture, VkSamplerAddressMode repeat_mode = VK_SAMPLER_ADDRESS_MODE_REPEAT, VkFilter filtering = VK_FILTER_NEAREST, VkImageAspectFlags aspect = VK_IMAGE_ASPECT_COLOR_BIT);
    PTImage* getTexture(uint16_t bind_point);
    inline bool getTextureUpdateFlag() { bool b = needs_texture_update; needs_texture_update = false; return b; }

    template <typename T>
    inline void setUniform(uint16_t bind_point, T data);
    template <typename T>
    inline T getUniform(uint16_t bind_point);

private:
    PTMaterial(VkDevice _device, std::string material_path, PTRenderPass* _render_pass, PTSwapchain* swapchain);
    PTMaterial(VkDevice _device, PTRenderPass* _render_pass, PTSwapchain* swapchain, PTShader* _shader, VkBool32 depth_write, VkBool32 depth_test, VkCompareOp depth_op, VkCullModeFlags culling, VkPolygonMode polygon_mode);
    ~PTMaterial();

    void initialiseMaterial(PTSwapchain* swapchain, VkBool32 depth_write, VkBool32 depth_test, VkCompareOp depth_op, VkCullModeFlags culling, VkPolygonMode polygon_mode);
};

#include "buffer.h"
#include <cstring>

template <typename T>
inline void PTMaterial::setUniform(uint16_t bind_point, T data)
{
    if (!uniform_buffers.contains(bind_point))
    {
        debugLog("WARNING: attempt to write to nonexistent uniform");
        return;
    }

    uint32_t buf_size = static_cast<uint32_t>(uniform_buffers[bind_point]->getSize());
    uint32_t data_size = static_cast<uint32_t>(sizeof(T));
    if (data_size < buf_size) buf_size = data_size;
    void* target = uniform_buffers[bind_point]->map();
    memcpy(target, &data, buf_size);
}

template <typename T>
inline T PTMaterial::getUniform(uint16_t bind_point)
{
    if (!uniform_buffers.contains(bind_point))
    {
        debugLog("WARNING: attempt to read from nonexistent uniform");
        return;
    }

    uint32_t buf_size = uniform_buffers[bind_point]->getSize();
    T data;
    void* target = uniform_buffers[bind_point]->map();
    memcpy(&data, target, buf_size);
}