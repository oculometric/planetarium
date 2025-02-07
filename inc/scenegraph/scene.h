#pragma once

#include <map>

#include "node.h"
#include "camera.h"
#include "resource.h"
#include "resource_manager.h"

class PTScene : PTResource
{
    friend class PTResourceManager;
private:
    std::map<std::string, PTResource*> referenced_resources;
    std::multimap<std::string, PTNode*> all_nodes;
    // TODO: always create this. objects should be resources too (allocated via the resource manager, but there should be a scene instantiate function. also, should use a unified constructor and an arugment map for simplicity), so they can use the referencing system. also the scene references the objects
    PTNode* root = nullptr;
    PTCameraNode* camera = nullptr;

public:
    PTScene(PTScene& other) = delete;
    PTScene(PTScene&& other) = delete;
    PTScene operator=(PTScene& other) = delete;
    PTScene operator=(PTScene&& other) = delete;

    template<class T>
    T* instantiate(std::string name, std::map<std::string, PTDeserialiser::Argument> arguments = { });
    // TODO: node destruction

    template<class T>
    T* getResource(std::string identifier);
    void addResource(std::string identifier, PTResource* resource);

    virtual void update(float delta_time);

    PTMatrix4f getCameraMatrix();

private:
// TODO: both of these
    PTScene();

    // should automatically un-depend on all nodes, and all resources
    ~PTScene();
};

template<class T>
inline T* PTScene::instantiate(std::string name, std::map<std::string, PTDeserialiser::Argument> arguments)
{
    static_assert(std::is_base_of<PTNode, T>::value, "T is not a PTNode type");
    PTNode* node = PTResourceManager::get()->createNode<T>(arguments);
    node->name = name;
    node->removeReferencer();

    addDependency(node);

    all_nodes.emplace(name, node);
    
    // TODO: parent to root

    return (T*)node;
}

template<class T>
inline T* PTScene::getResource(std::string identifier)
{
    static_assert(std::is_base_of<PTResource, T>::value, "T is not a PTResource type");
    if (referenced_resources.contains(identifier))
        return referenced_resources[identifier];
    return nullptr;
}
