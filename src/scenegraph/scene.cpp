#include "scene.h"

#include <iostream>

#include "application.h"
#include "debug.h"
#include "math/ptmath.h"
#include "graphics/resource_manager.h"
#include "input/input.h"

using namespace std;

PTScene::PTScene()
{
    root = instantiate<PTNode>("root");
    camera = nullptr;
}

PTScene::~PTScene()
{
    for (auto node : all_nodes)
        removeDependency(node.second);
    root = nullptr;
    camera = nullptr;
    all_nodes.clear();
    
    for (auto res : referenced_resources)
        removeDependency(res.second);
    referenced_resources.clear();
}

vector<PTNode*> PTScene::getNodes() const
{
    vector<PTNode*> nodes;
    nodes.reserve(all_nodes.size());
    for (auto pair : all_nodes)
        nodes.push_back(pair.second);
    return nodes;
}

void PTScene::addResource(std::string identifier, PTResource* resource)
{
    referenced_resources[identifier] = resource;
    addDependency(resource);
}

void PTScene::update(float delta_time)
{
    for (auto pair : all_nodes)
        pair.second->process(delta_time);
}

void PTScene::getCameraMatrix(float aspect_ratio, PTMatrix4f& world_to_view, PTMatrix4f& view_to_clip)
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