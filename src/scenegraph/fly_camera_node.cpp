#include "fly_camera_node.h"

#include <string>

using namespace std;

#include "input/input.h"
#include "render_server.h"

void PTFlyCameraNode::process(float delta_time)
{
    PTInput* manager = PTInput::get();

    // set debug mode
    PTRenderServer::get()->debug_mode = (manager->getKeyState('F').action == 1) || manager->getButtonState(PTGamepad::Button::CONTROL_SOUTH);
    // set screenshot wanted
    if ((manager->getKeyState('P').action == 1) || manager->getButtonState(PTGamepad::Button::CONTROL_NORTH))
        PTRenderServer::get()->setWantsScreenshot();

    // keyboard movement vector
    float keyboard_x = (float)(manager->getKeyState('D').action == 1) - (float)(manager->getKeyState('A').action == 1);
    float keyboard_y = (float)(manager->getKeyState('W').action == 1) - (float)(manager->getKeyState('S').action == 1);
    float keyboard_z = (float)(manager->getKeyState('E').action == 1) - (float)(manager->getKeyState('Q').action == 1);
    
    // keyboard look vector
    float keyboard_lx = (float)(manager->getKeyState('J').action == 1) - (float)(manager->getKeyState('L').action == 1);
    float keyboard_ly = (float)(manager->getKeyState('I').action == 1) - (float)(manager->getKeyState('K').action == 1);

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