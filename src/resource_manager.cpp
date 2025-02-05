#include "resource_manager.h"

#include "debug.h"

using namespace std;

static PTResourceManager* resource_manager = nullptr;

void PTResourceManager::init()
{
    if (resource_manager != nullptr)
        return;
    
    resource_manager = new PTResourceManager();
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

PTBuffer* PTResourceManager::createBuffer(VkDevice device, PTPhysicalDevice physical_device, VkDeviceSize buffer_size, VkBufferUsageFlags usage_flags, VkMemoryPropertyFlags memory_flags)
{
    string identifier = "buffer-" + to_string((size_t)device) + '-' + to_string((size_t)physical_device.getDevice()) + '-' + to_string(buffer_size) + '-' + to_string(usage_flags) + '-' + to_string(memory_flags);

    PTBuffer* buf = new PTBuffer(device, physical_device, buffer_size, usage_flags, memory_flags);
    resources.emplace(identifier, buf);

    buf->addReferencer();

    return buf;
}

PTImage* PTResourceManager::createImage(VkDevice device, PTPhysicalDevice physical_device, VkExtent2D size, VkFormat _format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties)
{
    string identifier = "image-" + to_string((size_t)device) + '-' + to_string((size_t)physical_device.getDevice()) + '-' + to_string(size.width) + '-' + to_string(size.height) + '-' + to_string(_format) + '-' + to_string(tiling) + '-' + to_string(usage) + '-' + to_string(properties);
    
    PTImage* img = new PTImage(device, physical_device, size, _format, tiling, usage, properties);
    resources.emplace(identifier, img);

    img->addReferencer();

    return img;
}

PTImage* PTResourceManager::createImage(VkDevice device, PTPhysicalDevice physical_device, std::string texture_file, bool force_duplicate)
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

PTMesh* PTResourceManager::createMesh(VkDevice device, const PTPhysicalDevice& physical_device, std::string file_name, bool force_duplicate)
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

PTMesh* PTResourceManager::createMesh(VkDevice device, const PTPhysicalDevice& physical_device, std::vector<PTVertex> vertices, std::vector<uint16_t> indices)
{
    string identifier = "mesh-" + to_string((size_t)device) + '-' + to_string((size_t)physical_device.getDevice()) + '-' + to_string(vertices.size()) + '-' + to_string(indices.size());

    PTMesh* me = new PTMesh(device, physical_device, vertices, indices);
    resources.emplace(identifier, me);

    me->addReferencer();

    return me;
}

PTPipeline* PTResourceManager::createPipeline(VkDevice device, PTShader* shader, PTRenderPass* render_pass, PTSwapchain* swapchain, VkBool32 depth_write, VkBool32 depth_test, VkCompareOp depth_op, VkCullModeFlags culling, VkFrontFace winding_order, VkPolygonMode polygon_mode, std::vector<VkDynamicState> dynamic_states)
{
    string identifier = "pipeline-" + to_string((size_t)device) + '-' + to_string((size_t)shader) + '-' + to_string((size_t)render_pass) + '-' + to_string((size_t)swapchain) + '-' + to_string(depth_write) + '-' + to_string(depth_test) + '-' + to_string(depth_op) + '-' + to_string(culling) + '-' + to_string(winding_order) + '-' + to_string(polygon_mode) + '-' + to_string(dynamic_states.size());
    
    PTPipeline* pipe = new PTPipeline(device, shader, render_pass, swapchain, depth_write, depth_test, depth_op, culling, winding_order, polygon_mode, dynamic_states);
    resources.emplace(identifier, pipe);

    pipe->addReferencer();

    return pipe;
}

PTRenderPass* PTResourceManager::createRenderPass(VkDevice device, std::vector<PTRenderPassAttachment> attachments, bool enable_depth)
{
    string identifier = "renderpass-" + to_string((size_t)device) + '-' + to_string(attachments.size()) + '-' + to_string(enable_depth);

    PTRenderPass* rp = new PTRenderPass(device, attachments, enable_depth);
    resources.emplace(identifier, rp);

    rp->addReferencer();

    return rp;
}

PTShader* PTResourceManager::createShader(VkDevice device, std::string shader_path_stub, bool force_duplicate)
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

PTSwapchain* PTResourceManager::createSwapchain(VkDevice device, PTPhysicalDevice& physical_device, VkSurfaceKHR surface, int window_x, int window_y)
{
    string identifier = "swapchain-" + '-' + to_string((size_t)device) + '-' + to_string((size_t)physical_device.getDevice()) + '-' + to_string((size_t)surface) + '-' + to_string(window_x) + '-' + to_string(window_y);

    PTSwapchain* sc = new PTSwapchain(device, physical_device, surface, window_x, window_y);
    resources.emplace(identifier, sc);

    sc->addReferencer();

    return sc;
}

PTResourceManager::~PTResourceManager()
{
    // TODO: force recompute reference counts

    while (!resources.empty())
    {
        auto iter = resources.begin();
        auto end = resources.end();
        while ((*iter).second->reference_counter != 0 && iter != end)
            iter++;
        
        if (iter != end || (*iter).second->reference_counter == 0)
        {
            resources.erase(iter);
            delete (*iter).second;
        }
        else
        {
            debugLog("error: unable to unload resources!");
            return;//throw runtime_error("unable to unload resources!");
        }
    }
}
