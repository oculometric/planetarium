#include "material.h"

#include "resource_manager.h"
#include "shader.h"
#include "buffer.h"
#include "pipeline.h"
#include "image.h"
#include "swapchain.h"

using namespace std;

bool checkParamExistsAndType(string name, PTShader::UniformType type, map<string, PTMaterial::MaterialParam> uniforms, PTMaterial::MaterialParam& out)
{
    auto it = uniforms.find(name);
    if (it == uniforms.end())
        return false;
    if (it->second.type != type)
        return false;
    out = it->second;
    return true;
}

float PTMaterial::getFloatParam(std::string name) const
{
    MaterialParam param;
    if (!checkParamExistsAndType(name, PTShader::UniformType::FLOAT, uniforms, param))
        return 0.0f;
    
    return param.vec_val.x;
}

PTVector2f PTMaterial::getVec2Param(std::string name) const
{
    MaterialParam param;
    if (!checkParamExistsAndType(name, PTShader::UniformType::VEC2, uniforms, param))
        return PTVector2f{ 0, 0 };
    
    return PTVector2f{ param.vec_val.x, param.vec_val.y };
}

PTVector3f PTMaterial::getVec3Param(std::string name) const
{
    MaterialParam param;
    if (!checkParamExistsAndType(name, PTShader::UniformType::VEC3, uniforms, param))
        return PTVector3f{ 0, 0, 0 };
    
    return PTVector3f{ param.vec_val.x, param.vec_val.y, param.vec_val.z };
}

PTVector4f PTMaterial::getVec4Param(std::string name) const
{
    MaterialParam param;
    if (!checkParamExistsAndType(name, PTShader::UniformType::VEC4, uniforms, param))
        return PTVector4f{ 0, 0, 0, 0 };
    
    return param.vec_val;
}

int PTMaterial::getIntParam(std::string name) const
{
    MaterialParam param;
    if (!checkParamExistsAndType(name, PTShader::UniformType::INT, uniforms, param))
        return 0;
    
    return param.int_val;
}

PTMatrix3f PTMaterial::getMat3Param(std::string name) const
{
    MaterialParam param;
    if (!checkParamExistsAndType(name, PTShader::UniformType::MAT3, uniforms, param))
        return PTMatrix3f();
    
    PTMatrix4f m = param.mat_val;
    return PTMatrix3f{ m.x_0, m.y_0, m.z_0,
                       m.x_1, m.y_1, m.z_1,
                       m.x_2, m.y_2, m.z_2 };
}

PTMatrix4f PTMaterial::getMat4Param(std::string name) const
{
    MaterialParam param;
    if (!checkParamExistsAndType(name, PTShader::UniformType::MAT4, uniforms, param))
        return PTMatrix4f();
    
    return param.mat_val;
}

PTImage* PTMaterial::getTextureParam(std::string name) const
{
    MaterialParam param;
    if (!checkParamExistsAndType(name, PTShader::UniformType::TEXTURE, uniforms, param))
        return nullptr;
    
    return param.tex_val;
}

void PTMaterial::setFloatParam(std::string name, float val)
{
    uniforms[name] = MaterialParam(val);
}

void PTMaterial::setVec2Param(std::string name, PTVector2f val)
{
    uniforms[name] = MaterialParam(val);
}

void PTMaterial::setVec3Param(std::string name, PTVector3f val)
{
    uniforms[name] = MaterialParam(val);
}

void PTMaterial::setVec4Param(std::string name, PTVector4f val)
{
    uniforms[name] = MaterialParam(val);
}

void PTMaterial::setIntParam(std::string name, int val)
{
    uniforms[name] = MaterialParam(val);
}

void PTMaterial::setMat3Param(std::string name, PTMatrix3f val)
{
    uniforms[name] = MaterialParam(val);
}

void PTMaterial::setMat4Param(std::string name, PTMatrix4f val)
{
    uniforms[name] = MaterialParam(val);
}

void PTMaterial::setTextureParam(std::string name, PTImage* val)
{
    uniforms[name] = MaterialParam(val);
}

PTMaterial::PTMaterial(VkDevice _device, PTSwapchain* swapchain, PTShader* _shader, std::map<std::string, MaterialParam> params, VkBool32 depth_write, VkBool32 depth_test, VkCompareOp depth_op, VkCullModeFlags culling, VkPolygonMode polygon_mode)
{
    device = _device;
    shader = _shader;
    uniforms = params;
    PTRenderPassAttachment attachment;
    attachment.format = swapchain->getImageFormat();
    render_pass = PTResourceManager::get()->createRenderPass({ attachment }, true);
    pipeline = PTResourceManager::get()->createPipeline(shader, render_pass, swapchain, depth_write, depth_test, depth_op, culling, VK_FRONT_FACE_COUNTER_CLOCKWISE, polygon_mode, { });

    addDependency(shader, true);
    addDependency(render_pass, false);
    addDependency(pipeline, false);
}

PTMaterial::~PTMaterial()
{
    removeDependency(render_pass);
    removeDependency(shader);
    removeDependency(pipeline);
}
