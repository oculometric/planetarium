#pragma once

#include <map>

#include "node.h"
#include "camera_node.h"
#include "resource_manager.h"

class PTScene : public PTResource
{
    friend class PTResourceManager;
private:
    std::map<std::string, PTResource*> referenced_resources;
    std::multimap<std::string, PTNode*> all_nodes;
    PTNode* root = nullptr;
    PTCameraNode* camera = nullptr;

public:
    PTScene(PTScene& other) = delete;
    PTScene(PTScene&& other) = delete;
    PTScene operator=(PTScene& other) = delete;
    PTScene operator=(PTScene&& other) = delete;

    template<class T>
    T* instantiate(std::string name, PTDeserialiser::ArgMap arguments = { });
    // TODO: node destruction (i.e. 'remove node from tree')

    template<class T>
    T* getResource(std::string identifier);
    void addResource(std::string identifier, PTResource* resource);

    void update(float delta_time);

    PTMatrix4f getCameraMatrix(float aspect_ratio);

private:
    PTScene();

    ~PTScene();
};

template<class T>
inline T* PTScene::instantiate(std::string name, PTDeserialiser::ArgMap arguments)
{
    static_assert(std::is_base_of<PTNode, T>::value, "T is not a PTNode type");
    PTNode* node = PTResourceManager::get()->createNode<T>(arguments);
    node->name = name;
    addDependency(node);
    node->removeReferencer();

    all_nodes.emplace(name, node);
    
    if (root != nullptr)
        node->getTransform()->setParent(root->getTransform());
    
    if (std::is_base_of<PTCameraNode, T>::value)
        camera = (PTCameraNode*)node;

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
