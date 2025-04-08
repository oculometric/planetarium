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
    std::map<uint16_t, PTBuffer*> descriptor_buffers;

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
    inline PTBuffer* getDescriptorBuffer(uint16_t binding) { return descriptor_buffers[binding]; }
    void applySetWrites(VkDescriptorSet descriptor_set);

    inline int getPriority() const { return priority; }
    inline void setPriority(int p) { priority = p; }

    template <typename T>
    inline void setUniform(uint16_t bind_point, T data);

    template <typename T>
    inline T getUniform(uint16_t bind_point);

private:
    PTMaterial(VkDevice _device, PTRenderPass* _render_pass, PTSwapchain* swapchain, PTShader* _shader, VkBool32 depth_write, VkBool32 depth_test, VkCompareOp depth_op, VkCullModeFlags culling, VkPolygonMode polygon_mode);
    ~PTMaterial();
};

#include "buffer.h"

template <typename T>
inline void PTMaterial::setUniform(uint16_t bind_point, T data)
{
    uint32_t buf_size = descriptor_buffers[bind_point]->getSize();
    void* target = descriptor_buffers[bind_point]->map();
    memcpy(target, &data, buf_size);
}

template <typename T>
inline T PTMaterial::getUniform(uint16_t bind_point)
{
    uint32_t buf_size = descriptor_buffers[bind_point]->getSize();
    T data;
    void* target = descriptor_buffers[bind_point]->map();
    memcpy(&data, target, buf_size);
}

// TODO: add path-loaded-from as a property for materials, shaders, images and meshes