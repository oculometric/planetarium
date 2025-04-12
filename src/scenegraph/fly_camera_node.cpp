#include "fly_camera_node.h"

#include <string>

using namespace std;

#include "input.h"
#include "render_server.h"
#include "scene.h"

void PTFlyCameraNode::process(float delta_time)
{
    PTNode* parent = getScene()->findNode<PTNode>("parent");
    parent->getTransform()->rotate(delta_time * 30.0f, PTVector3f::forward(), parent->getTransform()->getPosition());

    PTInput* manager = PTInput::get();

    // set debug mode
    PTRenderServer::get()->debug_mode = manager->wasKeyPressed('F') || manager->getButtonState(PTGamepad::Button::CONTROL_SOUTH);
    // set screenshot wanted
    if (manager->wasKeyPressed('P') || manager->getButtonState(PTGamepad::Button::CONTROL_NORTH))
        PTRenderServer::get()->setWantsScreenshot();

    // keyboard movement vector
    float keyboard_x = (float)(manager->isKeyDown('D')) - (float)(manager->isKeyDown('A'));
    float keyboard_y = (float)(manager->isKeyDown('W')) - (float)(manager->isKeyDown('S'));
    float keyboard_z = (float)(manager->isKeyDown('E')) - (float)(manager->isKeyDown('Q'));
    
    // keyboard look vector
    float keyboard_lx = (float)(manager->isKeyDown('J')) - (float)(manager->isKeyDown('L'));
    float keyboard_ly = (float)(manager->isKeyDown('I')) - (float)(manager->isKeyDown('K'));

    // gamepad move vector
    PTVector2f move_axis = manager->getJoystickState(PTGamepad::Axis::LEFT_AXIS) + PTVector2f{ keyboard_x, -keyboard_y };
    
    // construct local-space movement vector from combined move vector
    PTVector3f local_movement = PTVector3f
    {
       move_axis.x,
       (-((float)manager->getButtonState(PTGamepad::Button::LEFT_MAJOR) - (float)manager->getButtonState(PTGamepad::Button::RIGHT_MAJOR))) + keyboard_z,
       move_axis.y,
    } * delta_time * 2.0f;

    // rotate local movement vector to world space
    PTVector3f world_movement = rotate(getTransform()->getLocalRotation(), local_movement);

    // apply movement, and update the debug
    getTransform()->translate(world_movement);
    debugSetSceneProperty("camera pos", to_string(getTransform()->getLocalPosition()));
    
    // total look vector (keyboard and gamepad combined)
    PTVector2f look_axis = manager->getJoystickState(PTGamepad::Axis::RIGHT_AXIS) + PTVector2f{ -keyboard_lx, -keyboard_ly };
    debugSetSceneProperty("look", to_string(-look_axis.x) + ',' + to_string(-look_axis.y));

    // apply rotation according to look vector, update debug
    getTransform()->rotate(look_axis.y * delta_time * 90.0f, getTransform()->getRight(), getTransform()->getPosition());
    getTransform()->rotate(look_axis.x * delta_time * 90.0f, PTVector3f::forward(), getTransform()->getPosition());
    debugSetSceneProperty("camera rot", to_string(getTransform()->getLocalRotation()));

    debugSetSceneProperty("camera for", to_string(getTransform()->getForward()));

    // modify fov
    horizontal_fov += ((float)manager->getButtonState(PTGamepad::Button::RIGHT_MINOR) - (float)manager->getButtonState(PTGamepad::Button::LEFT_MINOR)) * delta_time * 30.0f;
    horizontal_fov = max(min(horizontal_fov, 120.0f), 10.0f);
    debugSetSceneProperty("camera fov", to_string(horizontal_fov));
}

PTFlyCameraNode::PTFlyCameraNode(PTDeserialiser::ArgMap arguments) : PTCameraNode(arguments)
{ }