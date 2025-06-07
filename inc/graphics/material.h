#pragma once

#include <vulkan/vulkan.h>
#include <string>
#include <map>

#include "reference_counter.h"
#include "constant.h"
#include "ptmath.h"
#include "shader.h"

typedef PTCountedPointer<class PTBuffer_T> PTBuffer;
typedef PTCountedPointer<class PTPipeline_T> PTPipeline;
typedef PTCountedPointer<class PTImage_T> PTImage;
typedef PTCountedPointer<class PTSampler_T> PTSampler;
typedef PTCountedPointer<class PTRenderPass_T> PTRenderPass;

class PTMaterial_T
{
private:
    VkDevice device = VK_NULL_HANDLE;
    PTShader shader = nullptr;
    PTRenderPass render_pass = nullptr;
    PTPipeline pipeline = nullptr;
    VkDescriptorPool descriptor_pool = VK_NULL_HANDLE;
    std::map<uint16_t, PTBuffer> uniform_buffers;
    std::map<uint16_t, std::pair<PTImage, std::pair<VkImageView, PTSampler>>> textures;
    bool needs_texture_update = false;

    int priority = 0;

    std::string origin_path;

public:
    PTMaterial_T(const PTMaterial_T& other) = delete;
    PTMaterial_T(const PTMaterial_T&& other) = delete;
    PTMaterial_T operator=(const PTMaterial_T& other) = delete;
    PTMaterial_T operator=(const PTMaterial_T&& other) = delete;
    ~PTMaterial_T();

    static inline PTCountedPointer<PTMaterial_T> createMaterial(std::string material_path)
    { return PTCountedPointer<PTMaterial_T>(new PTMaterial_T(material_path)); }
    static inline PTCountedPointer<PTMaterial_T> createMaterial(PTShader shader, VkBool32 depth_write, VkBool32 depth_test, VkCompareOp depth_op, VkCullModeFlags culling, VkPolygonMode polygon_mode)
    { return PTCountedPointer<PTMaterial_T>(new PTMaterial_T(shader, depth_write, depth_test, depth_op, culling, polygon_mode)); }

    PTShader getShader() const;
    PTRenderPass getRenderPass() const;
    PTPipeline getPipeline() const;
    PTBuffer getDescriptorBuffer(uint16_t binding);
    void applySetWrites(VkDescriptorSet descriptor_set);

    inline int getPriority() const { return priority; }
    inline void setPriority(int p) { priority = p; }

    void setTexture(uint16_t bind_point, PTImage texture, VkSamplerAddressMode repeat_mode = VK_SAMPLER_ADDRESS_MODE_REPEAT, VkFilter filtering = VK_FILTER_NEAREST, VkImageAspectFlags aspect = VK_IMAGE_ASPECT_COLOR_BIT);
    PTImage getTexture(uint16_t bind_point);
    inline bool getTextureUpdateFlag() { bool b = needs_texture_update; needs_texture_update = false; return b; }

    template <typename T>
    inline void setUniform(uint16_t bind_point, T data);
    template <typename T>
    inline T getUniform(uint16_t bind_point);

private:
    PTMaterial_T(std::string material_path);
    PTMaterial_T(PTShader _shader, VkBool32 depth_write, VkBool32 depth_test, VkCompareOp depth_op, VkCullModeFlags culling, VkPolygonMode polygon_mode);

    void initialiseMaterial(VkBool32 depth_write, VkBool32 depth_test, VkCompareOp depth_op, VkCullModeFlags culling, VkPolygonMode polygon_mode);
};

typedef PTCountedPointer<PTMaterial_T> PTMaterial;

#include "buffer.h"
#include <cstring>

template <typename T>
inline void PTMaterial_T::setUniform(uint16_t bind_point, T data)
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
inline T PTMaterial_T::getUniform(uint16_t bind_point)
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