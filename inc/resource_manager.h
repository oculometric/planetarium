#pragma once

#include <map>
#include <string>
#include <stdexcept>

#include "resource.h"
#include "buffer.h"
#include "image.h"
#include "mesh.h"
#include "pipeline.h"
#include "render_pass.h"
#include "shader.h"
#include "swapchain.h"

class PTResourceManager
{
private:
    std::multimap<std::string, PTResource*> resources;

public:
    static void init();
    static void deinit();
    static PTResourceManager* get();

    PTBuffer* createBuffer(VkDevice device, PTPhysicalDevice physical_device, VkDeviceSize buffer_size, VkBufferUsageFlags usage_flags, VkMemoryPropertyFlags memory_flags);
    PTImage* createImage(VkDevice device, PTPhysicalDevice physical_device, VkExtent2D size, VkFormat _format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties);
    PTImage* createImage(VkDevice device, PTPhysicalDevice physical_device, std::string texture_file, bool force_duplicate = false);
    PTMesh* createMesh(VkDevice device, const PTPhysicalDevice& physical_device, std::string file_name, bool force_duplicate = false);
    PTMesh* createMesh(VkDevice device, const PTPhysicalDevice& physical_device, std::vector<PTVertex> vertices, std::vector<uint16_t> indices);
    PTPipeline* createPipeline(VkDevice device, PTShader* shader, PTRenderPass* render_pass, PTSwapchain* swapchain, VkBool32 depth_write, VkBool32 depth_test, VkCompareOp depth_op, VkCullModeFlags culling, VkFrontFace winding_order, VkPolygonMode polygon_mode, std::vector<VkDynamicState> dynamic_states);
    PTRenderPass* createRenderPass(VkDevice device, std::vector<PTRenderPassAttachment> attachments, bool enable_depth);
    PTShader* createShader(VkDevice device, std::string shader_path_stub, bool force_duplicate = false);
    PTSwapchain* createSwapchain(VkDevice device, PTPhysicalDevice& physical_device, VkSurfaceKHR surface, int window_x, int window_y);

    template<typename T>
    void releaseResource(T* resource);

private:
    inline PTResourceManager() { };

    template<typename T>
    T* tryGetExistingResource(std::string identifier);
    
    ~PTResourceManager();
};

template<typename T>
inline void PTResourceManager::releaseResource(T* resource)
{
    if (resource->reference_counter == 0)
    {
        auto iter = resources.begin();
        auto end = resources.end();
        while ((*iter).second != resource && iter != end)
            iter++;

        if (iter == end && (*iter).second != resource)
            throw std::runtime_error("attempt to release a resource which was not registered!");

        resources.erase(iter);

        delete resource;
    }
}

template<typename T>
inline T* PTResourceManager::tryGetExistingResource(std::string identifier)
{
    if (resources.count(identifier) > 0)
        return (T*)((*(resources.find(identifier))).second);
    else
        return nullptr;
}
