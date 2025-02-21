#include "resource_manager.h"

#include <fstream>

#include "debug.h"
#include "scene.h"
#include "buffer.h"
#include "image.h"
#include "pipeline.h"
#include "shader.h"
#include "swapchain.h"

using namespace std;

static PTResourceManager* resource_manager = nullptr;

void PTResourceManager::init(VkDevice _device, PTPhysicalDevice& _physical_device)
{
    if (resource_manager != nullptr)
        return;
    
    resource_manager = new PTResourceManager(_device, _physical_device);
}

void PTResourceManager::deinit()
{
    if (resource_manager == nullptr)
        return;
    
    delete resource_manager;
}

PTResourceManager* PTResourceManager::get()
{
    return resource_manager;
}

PTBuffer* PTResourceManager::createBuffer(VkDeviceSize buffer_size, VkBufferUsageFlags usage_flags, VkMemoryPropertyFlags memory_flags)
{
    string identifier = "buffer-" + to_string((size_t)device) + '-' + to_string((size_t)physical_device.getDevice()) + '-' + to_string(buffer_size) + '-' + to_string(usage_flags) + '-' + to_string(memory_flags);

    PTBuffer* buf = new PTBuffer(device, physical_device, buffer_size, usage_flags, memory_flags);
    resources.emplace(identifier, buf);

    buf->addReferencer();

    return buf;
}

PTImage* PTResourceManager::createImage(VkExtent2D size, VkFormat _format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties)
{
    string identifier = "image-" + to_string((size_t)device) + '-' + to_string((size_t)physical_device.getDevice()) + '-' + to_string(size.width) + '-' + to_string(size.height) + '-' + to_string(_format) + '-' + to_string(tiling) + '-' + to_string(usage) + '-' + to_string(properties);
    
    PTImage* img = new PTImage(device, physical_device, size, _format, tiling, usage, properties);
    resources.emplace(identifier, img);

    img->addReferencer();

    return img;
}

PTImage* PTResourceManager::createImage(std::string texture_file, bool force_duplicate)
{
    string identifier = "image-" + to_string((size_t)device) + '-' + to_string((size_t)physical_device.getDevice()) + '-' + texture_file;

    PTImage* img = nullptr;
    if (!force_duplicate)
        img = tryGetExistingResource<PTImage>(identifier);
    if (img == nullptr)
        resources.emplace(identifier, img = new PTImage(device, physical_device, texture_file));

    img->addReferencer();
        
    return img;
}

PTMesh* PTResourceManager::createMesh(std::string file_name, bool force_duplicate)
{
    string identifier = "mesh-" + to_string((size_t)device) + '-' + to_string((size_t)physical_device.getDevice()) + '-' + file_name;

    PTMesh* me = nullptr;
    if (!force_duplicate)
        me = tryGetExistingResource<PTMesh>(identifier);
    if (me == nullptr)
        resources.emplace(identifier, me = new PTMesh(device, physical_device, file_name));

    me->addReferencer();

    return me;
}

PTMesh* PTResourceManager::createMesh(std::vector<PTVertex> vertices, std::vector<uint16_t> indices)
{
    string identifier = "mesh-" + to_string((size_t)device) + '-' + to_string((size_t)physical_device.getDevice()) + '-' + to_string(vertices.size()) + '-' + to_string(indices.size());

    PTMesh* me = new PTMesh(device, physical_device, vertices, indices);
    resources.emplace(identifier, me);

    me->addReferencer();

    return me;
}

PTPipeline* PTResourceManager::createPipeline(PTShader* shader, PTRenderPass* render_pass, PTSwapchain* swapchain, VkBool32 depth_write, VkBool32 depth_test, VkCompareOp depth_op, VkCullModeFlags culling, VkFrontFace winding_order, VkPolygonMode polygon_mode, std::vector<VkDynamicState> dynamic_states)
{
    string identifier = "pipeline-" + to_string((size_t)device) + '-' + to_string((size_t)shader) + '-' + to_string((size_t)render_pass) + '-' + to_string((size_t)swapchain) + '-' + to_string(depth_write) + '-' + to_string(depth_test) + '-' + to_string(depth_op) + '-' + to_string(culling) + '-' + to_string(winding_order) + '-' + to_string(polygon_mode) + '-' + to_string(dynamic_states.size());
    
    PTPipeline* pipe = new PTPipeline(device, shader, render_pass, swapchain, depth_write, depth_test, depth_op, culling, winding_order, polygon_mode, dynamic_states);
    resources.emplace(identifier, pipe);

    pipe->addReferencer();

    return pipe;
}

PTRenderPass* PTResourceManager::createRenderPass(std::vector<PTRenderPassAttachment> attachments, bool enable_depth)
{
    string identifier = "renderpass-" + to_string((size_t)device) + '-' + to_string(attachments.size()) + '-' + to_string(enable_depth);

    PTRenderPass* rp = new PTRenderPass(device, attachments, enable_depth);
    resources.emplace(identifier, rp);

    rp->addReferencer();

    return rp;
}

PTShader* PTResourceManager::createShader(std::string shader_path_stub, bool force_duplicate)
{
    string identifier = "shader-" + to_string((size_t)device) + '-' + shader_path_stub;

    PTShader* sh = nullptr;
    if (!force_duplicate)
        sh = tryGetExistingResource<PTShader>(identifier);
    if (sh == nullptr)
        resources.emplace(identifier, sh = new PTShader(device, shader_path_stub));

    sh->addReferencer();

    return sh;
}

PTSwapchain* PTResourceManager::createSwapchain(VkSurfaceKHR surface, int window_x, int window_y)
{
    string identifier = "swapchain-" + '-' + to_string((size_t)device) + '-' + to_string((size_t)physical_device.getDevice()) + '-' + to_string((size_t)surface) + '-' + to_string(window_x) + '-' + to_string(window_y);

    PTSwapchain* sc = new PTSwapchain(device, physical_device, surface, window_x, window_y);
    resources.emplace(identifier, sc);

    sc->addReferencer();

    return sc;
}

PTMaterial* PTResourceManager::createMaterial(PTSwapchain* swapchain, PTShader* _shader, std::map<std::string, PTMaterial::MaterialParam> params, VkBool32 depth_write, VkBool32 depth_test, VkCompareOp depth_op, VkCullModeFlags culling, VkPolygonMode polygon_mode)
{
    string identifier = "material-" + '-' + to_string((size_t)device) + '-' + to_string((size_t)_shader);

    PTMaterial* mt = new PTMaterial(device, swapchain, _shader, params, depth_write, depth_test, depth_op, culling, polygon_mode);
    resources.emplace(identifier, mt);

    mt->addReferencer();

    return mt;
}

PTScene* PTResourceManager::createScene(string file_name, bool force_duplicate)
{
    string identifier = "scene-" + file_name;
    
    PTScene* scene = nullptr;
    if (!force_duplicate)
        scene = tryGetExistingResource<PTScene>(identifier);
    if (scene == nullptr)
    {
        ifstream file(file_name, ios::ate);
        if (!file.is_open())
            return nullptr;

        size_t size = file.tellg();
        string scene_text;
        scene_text.resize(size, ' ');
        file.seekg(0);
        file.read(scene_text.data(), size);

        scene = new PTScene();
        PTDeserialiser::deserialiseScene(scene, scene_text);

        resources.emplace(identifier, scene);
    }

    scene->addReferencer();

    return scene;
}

PTResource* PTResourceManager::createGeneric(string type, vector<PTDeserialiser::Argument> args)
{
    if (type == "mesh")
    {
        if (args.size() < 1)
            return nullptr;
        if (args[0].type != PTDeserialiser::ArgType::STRING_ARG)
            return nullptr;

        return createMesh(args[0].s_val);
    }
    else if (type == "image")
    {
        if (args.size() < 1)
            return nullptr;
        if (args[0].type != PTDeserialiser::ArgType::STRING_ARG)
            return nullptr;

        return createImage(args[0].s_val);
    }
    // TODO: add other resource types as we go

    return nullptr;
}

PTResourceManager::~PTResourceManager()
{
    if (resources.empty())
    {
        debugLog("well done for cleaning up!");
        return;
    }

    debugLog("resource manager cleaning up " + to_string(resources.size()) + " lingering resources...");

    for (auto current_pair : resources)
    {
        PTResource* current = current_pair.second;
        current->reference_counter = 0;
        for (auto target_pair : resources)
        {
            if (target_pair.second == current)
                continue;
            if (target_pair.second->dependencies.contains(current))
                current->reference_counter++;
        }
    }

    while (!resources.empty())
    {
        auto iter = resources.begin();
        auto end = resources.end();
        while ((*iter).second->reference_counter != 0 && iter != end)
            iter++;
        
        if (iter != end || (*iter).second->reference_counter == 0)
        {
            debugLog("    unloading resource " + (*iter).first);
            resources.erase(iter);
            delete (*iter).second;
        }
        else
            break;
    }

    if (!resources.empty())
    {
        debugLog("    WARNING: cycle detected in resource dependency graph! unloads of the last " + to_string(resources.size()) + " resources may be out of order");
        for (auto pair : resources)
        {
            debugLog("    unloading resource " + pair.first);
            delete pair.second;
        }
    }
    resources.clear();

    debugLog("done.");
}
