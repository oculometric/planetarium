#pragma once

#include <vulkan/vulkan.h>
#include <string>
#include <map>

#include "resource.h"
#include "shader.h"
#include "buffer.h"
#include "pipeline.h"
#include "image.h"
#include "ptmath.h"

struct PTShader::UniformDescriptor
{
    std::string identifier;
    uint16_t offset;
    uint16_t size;
    PTShader::UniformType type;
};

class PTMaterial : public PTResource
{
public:
    struct MaterialParam
    {
        PTVector4f vec_val;
        int int_val;
        PTMatrix4f mat_val;
        PTImage* tex_val;

        PTShader::UniformType type;
    };

    friend class PTResourceManager;
private:
    VkDevice device = VK_NULL_HANDLE;
    PTShader* shader = nullptr;
    PTRenderPass* render_pass = nullptr;
    PTPipeline* pipeline = nullptr;
    // TODO: materials should also keep track of their descriptor set pool i think! since they should know what the descriptor set should look like for the shader (draw calls pull descriptor sets from the pool, i.e. not handled by material)
    VkDescriptorPool descriptor_pool = VK_NULL_HANDLE;

    int priority = 0; // TODO: draw queue ordering must respect material priority
    std::string origin_path;
    std::map<std::string, MaterialParam> uniforms;

    // TODO: keep track of descriptor set layout: names of uniforms, offsets, and expected types (also default values). detected from shader
    // TODO: integrate [https://github.com/KhronosGroup/SPIRV-Reflect]
    // TODO: in fact, actually the shader should track the uniform layout (list of descriptor bindings, i.e. uniform variables), and we just read it and use it to store the `uniforms` into the `buffer`

public:
    PTMaterial(const PTMaterial& other) = delete;
    PTMaterial(const PTMaterial&& other) = delete;
    PTMaterial operator=(const PTMaterial& other) = delete;
    PTMaterial operator=(const PTMaterial&& other) = delete;

    inline PTShader* getShader() const { return shader; }
    inline PTPipeline* getPipeline() const { return pipeline; }
    inline VkDescriptorPool getDescriptorPool() const { return descriptor_pool; }

    inline int getPriority() const { return priority; }
    inline void setPriority(int p) { priority = p; }

    // TODO: materials should be able to create a duplicate (i.e. instance) with modified MaterialParams
    PTMaterial* duplicate() const;

    float getFloatParam(std::string name) const;
    PTVector2f getVec2Param(std::string name) const;
    PTVector3f getVec3Param(std::string name) const;
    PTVector4f getVec4Param(std::string name) const;
    int getIntParam(std::string name) const;
    PTMatrix3f getMat3Param(std::string name) const;
    PTMatrix4f getMat4Param(std::string name) const;
    PTImage* getTextureParam(std::string name) const;

    void setFloatParam(std::string name, float val) const;
    void setVec2Param(std::string name, PTVector2f val) const;
    void setVec3Param(std::string name, PTVector3f val) const;
    void setVec4Param(std::string name, PTVector4f val) const;
    void setIntParam(std::string name, int val) const;
    void setMat3Param(std::string name, PTMatrix3f val) const;
    void setMat4Param(std::string name, PTMatrix4f val) const;
    void setTextureParam(std::string name, PTImage* val) const;

    // TODO: materials should handle updating their own uniform buffer
    void updateUniformBuffer(PTBuffer* buffer) const;

private:// TODO: on the material, i want to be able to configure the depth write/test/op, culling mode, and polygon mode, as well as the shader to use and the list of parameters to write to the uniform buffer
        // TODO: i also want a way to configure outputs from materials as well (what buffers they draw into? ie configuring the renderpass and pipeline attachments)
        // TODO: there should also be a way to specify special bindings for material parameters, like special textures (e.g the screen texture)
        // TODO: in other words the material should construct the pipeline and render pass (and descriptor set layout, and descriptor set pool) based on the shader and the other setup args
        // TODO: materials should be able to handle setting depth write/test/op, culling mode, polygon mode, attachments and shader (recreating stuff like pipeline, renderpass, descriptor set pool)
    PTMaterial(VkDevice _device, PTShader* _shader, std::map<std::string, MaterialParam> params, VkBool32 depth_write, VkBool32 depth_test, VkCompareOp depth_op, VkCullModeFlags culling, VkPolygonMode polygon_mode);
    PTMaterial();
    ~PTMaterial();
};

// TODO: add path-loaded-from as a property for shaders and meshes