#include "scene.h"

#include <iostream>

#include "application.h"
#include "debug.h"
#include "math/ptmath.h"
#include "graphics/resource_manager.h"
#include "input/input.h"

using namespace std;

PTScene_T::PTScene_T()
{
    // the root must always exist
    root = instantiate<PTNode>("root");
    camera = nullptr;
}

PTScene_T::~PTScene_T()
{
    // no longer depend on the nodes in the scene
    //for (auto node : all_nodes)
        //removeDependency(node.second);

    // clear references to root and camera, and all nodes
    all_nodes.clear();
    root = nullptr;
    camera = nullptr;
}

vector<PTNode*> PTScene_T::getNodes() const
{
    // copy all the nodes into a list
    vector<PTNode*> nodes;
    nodes.reserve(all_nodes.size());
    for (auto pair : all_nodes)
        nodes.push_back(pair.second);
    return nodes;
}

void PTScene_T::update(float delta_time)
{
    for (auto pair : all_nodes)
        pair.second->process(delta_time);
}

void PTScene_T::getCameraMatrix(float aspect_ratio, PTMatrix4f& world_to_view, PTMatrix4f& view_to_clip)
{
    if (camera == nullptr)
    {
        world_to_view = PTMatrix4f();
        view_to_clip = PTCameraNode::projectionMatrix(0.1f, 100.0f, 120.0f, aspect_ratio);
        return;
    }
    world_to_view = ~camera->getTransform()->getLocalToWorld();
    view_to_clip = camera->getProjectionMatrix(aspect_ratio);
}