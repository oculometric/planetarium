#pragma once

#include <map>
#include <vector>

#include "node.h"
#include "camera_node.h"
#include "resource_manager.h"
#include "resource.h"
#include "reference_counter.h"

class PTScene_T
{
private:
    std::multimap<std::string, PTNode*> all_nodes;
    PTNode* root = nullptr;
    PTCameraNode* camera = nullptr;

public:
    PTScene_T(PTScene_T& other) = delete;
    PTScene_T(PTScene_T&& other) = delete;
    PTScene_T operator=(PTScene_T& other) = delete;
    PTScene_T operator=(PTScene_T&& other) = delete;
    ~PTScene_T();

    static inline PTCountedPointer<PTScene_T> createScene()
    { return PTCountedPointer<PTScene_T>(new PTScene_T()); }
    static inline PTCountedPointer<PTScene_T> createScene(std::string scene_path)
    { return PTCountedPointer<PTScene_T>(new PTScene_T(scene_path)); }

    template<class T>
    T* instantiate(std::string name, PTDeserialiser::ArgMap arguments = { });
    // TODO: node destruction (i.e. 'remove node from tree')

    std::vector<PTNode*> getNodes() const;
    template<class T>
    T* findNode(std::string name) const;

    void update(float delta_time);

    inline PTCameraNode* getCamera() const { return camera; }
    void getCameraMatrix(float aspect_ratio, PTMatrix4f& world_to_view, PTMatrix4f& view_to_clip);

private:
    PTScene_T();
    PTScene_T(std::string scene_path);
};

typedef PTCountedPointer<PTScene_T> PTScene;

template<class T>
inline T* PTScene_T::instantiate(std::string name, PTDeserialiser::ArgMap arguments)
{
    static_assert(std::is_base_of<PTNode, T>::value, "T is not a PTNode type");
    PTNode* node = PTResourceManager::get()->createNode<T>(arguments);
    node->scene = this;
    node->name = name;

    all_nodes.emplace(name, node);
    
    if (root != nullptr)
        node->getTransform()->setParent(root->getTransform());
    
    if (std::is_base_of<PTCameraNode, T>::value)
        camera = (PTCameraNode*)node;

    return (T*)node;
}

template<class T>
inline T* PTScene_T::findNode(std::string name) const
{
    if (all_nodes.contains(name))
        return dynamic_cast<T*>(all_nodes.find(name)->second);
        
    return nullptr;
}