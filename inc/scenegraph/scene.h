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
    std::multimap<std::string, PTNode> all_nodes;
    PTNode root = nullptr;
    PTCameraNode camera = nullptr;

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
    PTCountedPointer<T> instantiate(std::string name, PTDeserialiser::ArgMap arguments = { });
    // TODO: node destruction (i.e. 'remove node from tree')

    std::vector<PTNode> getNodes() const;
    PTNode findNode(std::string name) const;

    void update(float delta_time);

    inline PTCameraNode getCamera() const { return camera; }
    void getCameraMatrix(float aspect_ratio, PTMatrix4f& world_to_view, PTMatrix4f& view_to_clip);

private:
    PTScene_T();
    PTScene_T(std::string scene_path);
};

typedef PTCountedPointer<PTScene_T> PTScene;

template<class T>
inline PTCountedPointer<T> PTScene_T::instantiate(std::string name, PTDeserialiser::ArgMap arguments)
{
    PTCountedPointer<T> node = PTNode_T::createNode<T>(name, this, arguments);

    all_nodes.emplace(name, node);
    // TODO: add children when this is refactored
    
    if (root != nullptr)
        node->getTransform()->setParent(root->getTransform());
    
    if (std::is_base_of<PTCameraNode_T, T>::value)
        camera = node;

    return node;
}