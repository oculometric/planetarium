#pragma once

#include <vulkan/vulkan.h>
#include <string>
#include <map>

#include "resource.h"
#include "ptmath.h"

class PTImage;
class PTBuffer;
class PTRenderPass;
class PTPipeline;
class PTShader;
class PTSwapchain;

class PTMaterial : public PTResource
{
public:
    struct MaterialParam
    {
        PTVector4f vec_val{ 0, 0, 0, 0 };
        int int_val = 0;
        PTMatrix4f mat_val;
        PTImage* tex_val = nullptr;

        PTShader::UniformType type;

        inline MaterialParam() { }
        inline MaterialParam(float val) { vec_val = PTVector4f{ val, 0, 0, 0 }; type = PTShader::UniformType::FLOAT; }
        inline MaterialParam(PTVector2f val) { vec_val = PTVector4f{ val.x, val.y, 0, 0 }; type = PTShader::UniformType::VEC2; }
        inline MaterialParam(PTVector3f val) { vec_val = PTVector4f{ val.x, val.y, val.z, 0 }; type = PTShader::UniformType::VEC3; }
        inline MaterialParam(PTVector4f val) { vec_val = PTVector4f{ val.x, val.y, val.z, val.w }; type = PTShader::UniformType::VEC4; }
        inline MaterialParam(int val) { int_val = val; type = PTShader::UniformType::INT; }
        inline MaterialParam(PTMatrix3f val) { mat_val = PTMatrix4f{ val.x_0, val.y_0, val.z_0, 0,
                                                                     val.x_1, val.y_1, val.z_1, 0,
                                                                     val.x_2, val.y_2, val.z_2, 0,
                                                                     0,       0,       0,       1 }; type = PTShader::UniformType::MAT3; }
        inline MaterialParam(PTMatrix4f val) { mat_val = val; type = PTShader::UniformType::MAT4; }
        inline MaterialParam(PTImage* val) { tex_val = val; type = PTShader::UniformType::TEXTURE; }
    };

    friend class PTResourceManager;
private:
    VkDevice device = VK_NULL_HANDLE;
    PTShader* shader = nullptr;
    PTRenderPass* render_pass = nullptr;
    PTPipeline* pipeline = nullptr;

    int priority = 0;
    std::string origin_path;
    std::map<std::string, MaterialParam> uniforms;

public:
    PTMaterial(const PTMaterial& other) = delete;
    PTMaterial(const PTMaterial&& other) = delete;
    PTMaterial operator=(const PTMaterial& other) = delete;
    PTMaterial operator=(const PTMaterial&& other) = delete;

    inline PTShader* getShader() const { return shader; }
    inline PTRenderPass* getRenderPass() const { return render_pass; }
    inline PTPipeline* getPipeline() const { return pipeline; }

    inline int getPriority() const { return priority; }
    inline void setPriority(int p) { priority = p; }

    float getFloatParam(std::string name) const;
    PTVector2f getVec2Param(std::string name) const;
    PTVector3f getVec3Param(std::string name) const;
    PTVector4f getVec4Param(std::string name) const;
    int getIntParam(std::string name) const;
    PTMatrix3f getMat3Param(std::string name) const;
    PTMatrix4f getMat4Param(std::string name) const;
    PTImage* getTextureParam(std::string name) const;

    void setFloatParam(std::string name, float val);
    void setVec2Param(std::string name, PTVector2f val);
    void setVec3Param(std::string name, PTVector3f val);
    void setVec4Param(std::string name, PTVector4f val);
    void setIntParam(std::string name, int val);
    void setMat3Param(std::string name, PTMatrix3f val);
    void setMat4Param(std::string name, PTMatrix4f val);
    void setTextureParam(std::string name, PTImage* val);

    // TODO: materials should handle updating their own uniform buffer
    void updateUniformBuffer(PTBuffer* buffer) const;

private:
    // TODO: i also want a way to configure outputs from materials as well (what buffers they draw into? ie configuring the renderpass and pipeline attachments)
    // TODO: there should also be a way to specify special bindings for material parameters, like special textures (e.g the screen texture)
    PTMaterial(VkDevice _device, PTSwapchain* swapchain, PTShader* _shader, std::map<std::string, MaterialParam> params, VkBool32 depth_write, VkBool32 depth_test, VkCompareOp depth_op, VkCullModeFlags culling, VkPolygonMode polygon_mode);
    PTMaterial(std::string load_path); // TODO: loading material from file
    ~PTMaterial();
};

// TODO: add path-loaded-from as a property for shaders and meshes