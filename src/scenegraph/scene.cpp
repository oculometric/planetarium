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

    // ie this code would go on a subclass of the PTCameraNode class
    PTInput* manager = PTInput::get();

    PTApplication::get()->debug_mode = (manager->getKeyState('F').action == 1) || manager->getButtonState(PTGamepad::Button::CONTROL_SOUTH);
    PTApplication::get()->wants_screenshot = (manager->getKeyState('P').action == 1) || manager->getButtonState(PTGamepad::Button::CONTROL_NORTH);

    float keyboard_x = (float)(manager->getKeyState('D').action == 1) - (float)(manager->getKeyState('A').action == 1);
    float keyboard_y = (float)(manager->getKeyState('W').action == 1) - (float)(manager->getKeyState('S').action == 1);
    float keyboard_z = (float)(manager->getKeyState('E').action == 1) - (float)(manager->getKeyState('Q').action == 1);
    
    float keyboard_lx = (float)(manager->getKeyState('J').action == 1) - (float)(manager->getKeyState('L').action == 1);
    float keyboard_ly = (float)(manager->getKeyState('I').action == 1) - (float)(manager->getKeyState('K').action == 1);

    PTVector2f move_axis = manager->getJoystickState(PTGamepad::Axis::LEFT_AXIS) + PTVector2f{ keyboard_x, -keyboard_y };
    PTVector3f local_movement = PTVector3f
    {
       move_axis.x,
       (-((float)manager->getButtonState(PTGamepad::Button::LEFT_MAJOR) - (float)manager->getButtonState(PTGamepad::Button::RIGHT_MAJOR))) + keyboard_z,
       move_axis.y,
    } * delta_time * 2.0f;

    PTVector3f world_movement = rotate(camera->getTransform()->getLocalRotation(), local_movement);

    camera->getTransform()->translate(world_movement);
    debugSetSceneProperty("camera pos", to_string(camera->getTransform()->getLocalPosition()));
    
    PTVector2f look_axis = manager->getJoystickState(PTGamepad::Axis::RIGHT_AXIS) + PTVector2f{ -keyboard_lx, -keyboard_ly };
    debugSetSceneProperty("look", to_string(-look_axis.x) + ',' + to_string(-look_axis.y));

    camera->getTransform()->rotate(look_axis.y * delta_time * 90.0f, camera->getTransform()->getRight(), camera->getTransform()->getPosition());
    camera->getTransform()->rotate(look_axis.x * delta_time * 90.0f, PTVector3f::forward(), camera->getTransform()->getPosition());
    debugSetSceneProperty("camera rot", to_string(camera->getTransform()->getLocalRotation()));

    debugSetSceneProperty("camera for", to_string(camera->getTransform()->getForward()));

    camera->horizontal_fov += ((float)manager->getButtonState(PTGamepad::Button::RIGHT_MINOR) - (float)manager->getButtonState(PTGamepad::Button::LEFT_MINOR)) * delta_time * 30.0f;
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