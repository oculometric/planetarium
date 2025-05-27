#include "resource_manager.h"

#include <fstream>

#include "debug.h"
#include "scene.h"
#include "buffer.h"
#include "image.h"
#include "pipeline.h"
#include "shader.h"
#include "swapchain.h"
#include "render_server.h"
#include "sampler.h"

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
    resource_manager = nullptr;
}

PTResourceManager* PTResourceManager::get()
{
    return resource_manager;
}

PTBuffer* PTResourceManager::createBuffer(VkDeviceSize buffer_size, VkBufferUsageFlags usage_flags, VkMemoryPropertyFlags memory_flags)
{
    PTBuffer* buf = new PTBuffer(device, physical_device, buffer_size, usage_flags, memory_flags);
    string identifier = "buffer-" + to_string((size_t)buf);

    resources.emplace(identifier, buf);

    buf->addReferencer();

    return buf;
}

PTImage* PTResourceManager::createImage(VkExtent2D size, VkFormat _format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties)
{
    PTImage* img = new PTImage(device, physical_device, size, _format, tiling, usage, properties);
    string identifier = "image-" + to_string((size_t)img);

    resources.emplace(identifier, img);

    img->addReferencer();

    return img;
}

PTImage* PTResourceManager::createImage(std::string texture_file, bool force_duplicate)
{
    string identifier = "image-" + texture_file;
    PTImage* img = nullptr;

    if (!force_duplicate)
        img = tryGetExistingResource<PTImage>(identifier);
    if (img == nullptr)
        resources.emplace(identifier, img = new PTImage(device, texture_file, physical_device));

    img->addReferencer();
        
    return img;
}

PTMesh* PTResourceManager::createMesh(std::string file_name, bool force_duplicate)
{
    string identifier = "mesh-" + file_name;
    PTMesh* me = nullptr;

    if (!force_duplicate)
        me = tryGetExistingResource<PTMesh>(identifier);
    if (me == nullptr)
        resources.emplace(identifier, me = new PTMesh(device, file_name, physical_device));

    me->addReferencer();

    return me;
}

PTMesh* PTResourceManager::createMesh(std::vector<PTVertex> vertices, std::vector<uint16_t> indices)
{
    PTMesh* me = new PTMesh(device, vertices, indices, physical_device);
    string identifier = "mesh-" + to_string((size_t)me);

    resources.emplace(identifier, me);

    me->addReferencer();

    return me;
}

PTPipeline* PTResourceManager::createPipeline(PTShader* shader, PTRenderPass* render_pass, PTSwapchain* swapchain, VkBool32 depth_write, VkBool32 depth_test, VkCompareOp depth_op, VkCullModeFlags culling, VkFrontFace winding_order, VkPolygonMode polygon_mode, std::vector<VkDynamicState> dynamic_states)
{
    PTPipeline* pipe = new PTPipeline(device, shader, render_pass, swapchain, depth_write, depth_test, depth_op, culling, winding_order, polygon_mode, dynamic_states);
    string identifier = "pipeline-" + to_string((size_t)pipe);
    
    resources.emplace(identifier, pipe);

    pipe->addReferencer();

    return pipe;
}

PTRenderPass* PTResourceManager::createRenderPass(std::vector<PTRenderPass::Attachment> attachments, bool transition_to_readable)
{
    PTRenderPass* rp = new PTRenderPass(device, attachments, transition_to_readable);
    string identifier = "renderpass-" + to_string((size_t)rp);
    
    resources.emplace(identifier, rp);

    rp->addReferencer();

    return rp;
}

PTShader* PTResourceManager::createShader(std::string shader_path_stub, bool is_precompiled, bool has_geometry_shader, bool force_duplicate)
{
    string identifier = "shader-" + shader_path_stub;
    PTShader* sh = nullptr;

    if (!force_duplicate)
        sh = tryGetExistingResource<PTShader>(identifier);
    if (sh == nullptr)
        resources.emplace(identifier, sh = new PTShader(device, shader_path_stub, is_precompiled, has_geometry_shader));

    sh->addReferencer();

    return sh;
}

PTSwapchain* PTResourceManager::createSwapchain(VkSurfaceKHR surface, int window_x, int window_y)
{
    string identifier = "swapchain-" + to_string((size_t)surface) + '-' + to_string(window_x) + '-' + to_string(window_y);

    PTSwapchain* sc = new PTSwapchain(device, physical_device, surface, window_x, window_y);
    resources.emplace(identifier, sc);

    sc->addReferencer();

    return sc;
}

PTMaterial* PTResourceManager::createMaterial(std::string material_path, PTSwapchain* swapchain, PTRenderPass* render_pass, bool force_duplicate)
{
    string identifier = "material-" + material_path;
    PTMaterial* mt = nullptr;

    if (!force_duplicate)
        mt = tryGetExistingResource<PTMaterial>(identifier);
    if (mt == nullptr)
        resources.emplace(identifier, mt = new PTMaterial(device, material_path, render_pass == nullptr ? PTRenderServer::get()->getRenderPass() : render_pass, swapchain == nullptr ? PTRenderServer::get()->getSwapchain() : swapchain));

    mt->addReferencer();

    return mt;
}

PTMaterial* PTResourceManager::createMaterial(PTSwapchain* swapchain, PTRenderPass* _render_pass, PTShader* _shader, VkBool32 depth_write, VkBool32 depth_test, VkCompareOp depth_op, VkCullModeFlags culling, VkPolygonMode polygon_mode)
{
    PTMaterial* mt = new PTMaterial(device, _render_pass, swapchain, _shader, depth_write, depth_test, depth_op, culling, polygon_mode);
    string identifier = "material-" + '-' + to_string((size_t)mt) + '-' + to_string((size_t)_shader);
    resources.emplace(identifier, mt);

    mt->addReferencer();

    return mt;
}

PTSampler* PTResourceManager::createSampler(VkSamplerAddressMode _address_mode, VkFilter _min_filter, VkFilter _mag_filter, uint32_t _max_anisotropy, bool force_duplicate)
{
    string identifier = "sampler-" + to_string(_address_mode) + '-' + to_string(_min_filter) + '-' + to_string(_mag_filter) + '-' + to_string(_max_anisotropy);
    PTSampler* sm;

    if (!force_duplicate)
        sm = tryGetExistingResource<PTSampler>(identifier);
    if (sm == nullptr)
        resources.emplace(identifier, sm = new PTSampler(device, _address_mode, _min_filter, _mag_filter, _max_anisotropy));

    sm->addReferencer();

    return sm;
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
    else if (type == "shader")
    {
        if (args.size() < 1)
            return nullptr;
        if (args[0].type != PTDeserialiser::ArgType::STRING_ARG)
            return nullptr;
        
        return createShader(args[0].s_val, false);
    }
    else if (type == "material")
    {
        if (args.size() < 1)
            return nullptr;
        if (args[0].type != PTDeserialiser::ArgType::STRING_ARG)
            return nullptr;
        
        return createMaterial(args[0].s_val);
    }

    return nullptr;
}

PTResourceManager::~PTResourceManager()
{
    debugLog("shutting down resource manager.");

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
        while (iter != end && (*iter).second->reference_counter != 0)
            iter++;

        if (iter == end)
            break;
        
        debugLog("    unloading resource " + (*iter).first);
        PTResource* res = (*iter).second;
        resources.erase(iter);
        delete res;
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
