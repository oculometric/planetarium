#include "scene.h"

#include <iostream>

#include "application.h"
#include "debug.h"
#include "ptmath.h"
#include "resource_manager.h"

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

void PTScene::addResource(std::string identifier, PTResource* resource)
{
    referenced_resources[identifier] = resource;
    addDependency(resource);
}

// TODO: fix all this. the scene shouldn't have functionality, the scene should only manage the nodes and resources. the *nodes* should be the ones with functionality, which the user can override
void PTScene::update(float delta_time)
{
    for (auto pair : all_nodes)
        pair.second->process(delta_time);

    PTInputManager* manager = PTApplication::get()->getInputManager();

    PTApplication::get()->debug_mode = (manager->getKeyState('D').action == 1) || manager->getButtonState(PTInputButton::CONTROL_SOUTH);
    PTApplication::get()->wants_screenshot = (manager->getKeyState('P').action == 1) || manager->getButtonState(PTInputButton::CONTROL_NORTH);

    PTVector3f local_movement = PTVector3f
    {
       (float)manager->getAxisState(PTInputAxis::MOVE_AXIS_X) / (float)INT16_MAX,
       -((float)manager->getButtonState(PTInputButton::LEFT_MAJOR) - (float)manager->getButtonState(PTInputButton::RIGHT_MAJOR)),
       (float)manager->getAxisState(PTInputAxis::MOVE_AXIS_Y) / (float)INT16_MAX,
    } * delta_time * 2.0f;

    // ie this code would go on a subclass of the PTCameraNode class
    PTVector3f world_movement = rotate(camera->getTransform()->getLocalRotation(), local_movement);

    camera->getTransform()->translate(world_movement);
    debugSetSceneProperty("camera pos", to_string(camera->getTransform()->getLocalPosition()));
    
    float look_x = (float)manager->getAxisState(PTInputAxis::LOOK_AXIS_X) / INT16_MAX;
    float look_y = (float)manager->getAxisState(PTInputAxis::LOOK_AXIS_Y) / INT16_MAX;
    debugSetSceneProperty("look", to_string(-look_x) + ',' + to_string(-look_y));

    camera->getTransform()->rotate(look_y * delta_time * 90.0f, camera->getTransform()->getRight(), camera->getTransform()->getPosition());
    camera->getTransform()->rotate(look_x * delta_time * 90.0f, PTVector3f::forward(), camera->getTransform()->getPosition());
    debugSetSceneProperty("camera rot", to_string(camera->getTransform()->getLocalRotation()));

    debugSetSceneProperty("camera for", to_string(camera->getTransform()->getForward()));

    camera->horizontal_fov += ((float)manager->getButtonState(PTInputButton::RIGHT_MINOR) - (float)manager->getButtonState(PTInputButton::LEFT_MINOR)) * delta_time * 30.0f;
    camera->horizontal_fov = max(min(camera->horizontal_fov, 120.0f), 10.0f);
    debugSetSceneProperty("camera fov", to_string(camera->horizontal_fov));
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