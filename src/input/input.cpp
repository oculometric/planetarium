#include "input/input.h"

#include "debug.h"

using namespace std;

PTInput* input_manager = nullptr;

PTInput* PTInput::get()
{
    return input_manager;
}

bool PTInput::getButtonState(PTGamepad::Button button, uint8_t gamepad) const
{
    if (gamepad >= gamepads.size())
        return false;
    return gamepads[gamepad].getButtonState(button);
}

PTVector2f PTInput::getJoystickState(PTGamepad::Axis axis, uint8_t gamepad) const
{
    if (gamepad >= gamepads.size())
        return PTVector2f{ 0, 0 };
    return gamepads[gamepad].getJoystickState(axis);
}

bool PTInput::getButtonState(PTGamepad::Button button) const
{
    bool b = false;
    for (uint8_t i = 0; i < gamepads.size(); i++)
        b = b || gamepads[i].getButtonState(button);
    return b;
}

PTVector2f PTInput::getJoystickState(PTGamepad::Axis axis) const
{
    PTVector2f max{ 0, 0 };
    for (uint8_t i = 0; i < gamepads.size(); i++)
    {
        PTVector2f val = gamepads[i].getJoystickState(axis);
        if (mag(val) > mag(max)) max = val;
    }
    return max;
}

void PTInput::handleKeyboardEvent(int key, int action, int mods)
{
    translate(key, action, mods);
}

PTKeyState PTInput::getKeyState(int key) const
{
    if (key_states.contains(key))
        return key_states.at(key);
    return PTKeyState{ key, 0, 0 };
}

PTInput::PTInput()
{
    if (input_manager != nullptr)
        throw runtime_error("attempt to create duplicate input manager");

    input_manager = this;

    for (uint8_t i = 0; i < 4; i++)
        gamepads[i] = PTGamepad(i);
}

void PTInput::pollGamepads()
{
    for (uint8_t i = 0; i < gamepads.size(); i++)
    {
        if (gamepads[i].isValid())
            while (gamepads[i].poll().is_valid);
        else
            gamepads[i] = PTGamepad(i);
    }
}

void PTInput::translate(int key, int action, int mods)
{
    if (action == 2) return;
    
    debugLog("keyboard event: key (" + to_string(key) + "), action (" + to_string(action) + "), mods (" + to_string(mods) + ")");
    key_states[key] = PTKeyState{ key, action, mods };
}

void keyboardCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (input_manager) input_manager->handleKeyboardEvent(key, action, mods);
}