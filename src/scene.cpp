#include "scene.h"

#include <iostream>

#include "application.h"
#include "debug.h"

using namespace std;

PTScene::PTScene(PTApplication* application)
{
    owner = application;
    camera.local_position = OLVector3f{ 0, 0, 1 };
    camera.local_rotation = OLVector3f{ 0, 0, 0 };
    camera.local_scale = OLVector3f{ 1, 1, 1 };
    camera.horizontal_fov = 90.0f;
    camera.aspect_ratio = 4.0f / 3.0f;
    camera.far_clip = 10.0f;
    camera.near_clip = 0.1f;
}

void PTScene::update(float delta_time)
{
    PTInputManager* manager = owner->getInputManager();

    PTApplication::get()->debug_mode = manager->getKeyState('D').action == 1;

    OLVector4f local_movement = OLVector4f
    {
        (float)manager->getAxisState(PTInputAxis::MOVE_AXIS_X) / (float)INT16_MAX,
        -((float)manager->getButtonState(PTInputButton::LEFT_MAJOR) - (float)manager->getButtonState(PTInputButton::RIGHT_MAJOR)),
        (float)manager->getAxisState(PTInputAxis::MOVE_AXIS_Y) / (float)INT16_MAX, 
        0
    } * delta_time * 2.0f;

    OLVector4f world_movement = camera.getLocalRotation() * local_movement;

    camera.local_position += OLVector3f{ world_movement.x, world_movement.y, world_movement.z };
    debugSetSceneProperty("camera pos", to_string(camera.local_position));
    
    camera.local_rotation -= (OLVector3f{ (float)manager->getAxisState(PTInputAxis::LOOK_AXIS_Y), 0, (float)manager->getAxisState(PTInputAxis::LOOK_AXIS_X) } / INT16_MAX) * delta_time * 90.0f;
    debugSetSceneProperty("camera rot", to_string(camera.local_rotation));

    camera.horizontal_fov += ((float)manager->getButtonState(PTInputButton::RIGHT_MINOR) - (float)manager->getButtonState(PTInputButton::LEFT_MINOR)) * delta_time * 30.0f;
    camera.horizontal_fov = max(min(camera.horizontal_fov, 120.0f), 10.0f);
    debugSetSceneProperty("camera fov", to_string(camera.horizontal_fov));
    debugSetSceneProperty("camera asp", to_string(camera.aspect_ratio));
    
}

OLMatrix4f PTScene::getCameraMatrix()
{
    return camera.getProjectionMatrix() * ~camera.getLocalTransform();
}