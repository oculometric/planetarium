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
#include "node.h"
#include "deserialiser.h"

class PTResourceManager
{
private:
    VkDevice device = VK_NULL_HANDLE;
    PTPhysicalDevice& physical_device;

    std::multimap<std::string, PTResource*> resources;

public:
    static void init(VkDevice _device, PTPhysicalDevice& _physical_device);
    static void deinit();
    static PTResourceManager* get();

    PTBuffer* createBuffer(VkDeviceSize buffer_size, VkBufferUsageFlags usage_flags, VkMemoryPropertyFlags memory_flags);
    PTImage* createImage(VkExtent2D size, VkFormat _format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties);
    PTImage* createImage(std::string texture_file, bool force_duplicate = false);
    PTMesh* createMesh(std::string file_name, bool force_duplicate = false);
    PTMesh* createMesh(std::vector<PTVertex> vertices, std::vector<uint16_t> indices);
    PTPipeline* createPipeline(PTShader* shader, PTRenderPass* render_pass, PTSwapchain* swapchain, VkBool32 depth_write, VkBool32 depth_test, VkCompareOp depth_op, VkCullModeFlags culling, VkFrontFace winding_order, VkPolygonMode polygon_mode, std::vector<VkDynamicState> dynamic_states);
    PTRenderPass* createRenderPass(std::vector<PTRenderPassAttachment> attachments, bool enable_depth);
    PTShader* createShader(std::string shader_path_stub, bool force_duplicate = false);
    PTSwapchain* createSwapchain(VkSurfaceKHR surface, int window_x, int window_y);

    template<class T>
    PTNode* createNode(std::map<std::string, PTDeserialiser::Argument> arguments);

    PTResource* createGeneric(std::string type, std::vector<PTDeserialiser::Argument> args);

    template<typename T>
    void releaseResource(T* resource);

private:
    inline PTResourceManager(VkDevice _device, PTPhysicalDevice& _physical_device) : device(_device), physical_device(_physical_device) { };

    template<typename T>
    T* tryGetExistingResource(std::string identifier);
    
    ~PTResourceManager();
};

template<class T>
inline PTNode* PTResourceManager::createNode(std::map<std::string, PTDeserialiser::Argument> arguments)
{
    static_assert(std::is_base_of<PTNode, T>::value, "T is not a PTNode type");
    T* node = new T(arguments);

    resources.emplace(to_string(node), node);
    node->addReferencer();

    return node;
}

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
