#pragma once

#include <map>
#include <string>
#include <stdexcept>

#include "resource.h"
#include "physical_device.h"
#include "node.h"
#include "deserialiser.h"

class PTResourceManager
{
private:
    VkDevice device = VK_NULL_HANDLE;
    PTPhysicalDevice& physical_device;

    //std::multimap<std::string, PTResource*> resources;

public:
    static void init(VkDevice _device, PTPhysicalDevice& _physical_device);
    static void deinit();
    static PTResourceManager* get();

    //template<class T>
    //PTCountedPointer<T> createNode(PTDeserialiser::ArgMap arguments);

    //template<typename T>
    //void releaseResource(T* resource);

private:
    inline PTResourceManager(VkDevice _device, PTPhysicalDevice& _physical_device) : device(_device), physical_device(_physical_device) { };

    //template<typename T>
    //T* tryGetExistingResource(std::string identifier);
    
    ~PTResourceManager();
};

//template<class T>
//inline PTCountedPointer<T> PTResourceManager::createNode(PTDeserialiser::ArgMap arguments)
//{
//    static_assert(std::is_base_of<PTNode_T, T>::value, "T is not a PTNode_T type");
//    PTCountedPointer<T> node = PTCountedPointer<T>(new T(arguments));
//
//    //resources.emplace("node-" + std::to_string((size_t)node), node);
//    //node->addReferencer();
//
//    return node;
//}
//
//template<typename T>
//inline void PTResourceManager::releaseResource(T* resource)
//{
//    if (resource->reference_counter == 0)
//    {
//        auto iter = resources.begin();
//        auto end = resources.end();
//        while ((*iter).second != resource && iter != end)
//            iter++;
//
//        if (iter == end && (*iter).second != resource)
//            throw std::runtime_error("attempt to release a resource which was not registered!");
//
//        resources.erase(iter);
//
//        delete resource;
//    }
//}
//
//template<typename T>
//inline T* PTResourceManager::tryGetExistingResource(std::string identifier)
//{
//    if (resources.count(identifier) > 0)
//        return (T*)((*(resources.find(identifier))).second);
//    else
//        return nullptr;
//}
