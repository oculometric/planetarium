#include "input/input.h"

#include <GLFW/glfw3.h>

#include "debug.h"
#include "input.h"

using namespace std;

PTInput* input_manager = nullptr;

void PTInput::init(GLFWwindow* window)
{
    if (input_manager != nullptr)
        return;

    input_manager = new PTInput(window);
}

void PTInput::deinit()
{
    if (input_manager == nullptr)
        return;

    delete input_manager;
    input_manager = nullptr;
}

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
    Modifiers modifiers = Modifiers::NONE;
    if (mods & GLFW_MOD_SHIFT)
        modifiers = (Modifiers)(modifiers | Modifiers::SHIFT);
    if (mods & GLFW_MOD_CONTROL)
        modifiers = (Modifiers)(modifiers | Modifiers::CTRL);
    if (mods & GLFW_MOD_ALT)
        modifiers = (Modifiers)(modifiers | Modifiers::ALT);
    
    if (action == GLFW_RELEASE)
        key_states[key] = StateInfo{ State::RELEASED, Modifiers::NONE };
    else if (action == GLFW_PRESS)
        key_states[key] = StateInfo{ (State)(State::PRESSED | State::DOWN), modifiers };
    else if (action == GLFW_REPEAT)
    {
        if (wasKeyPressed(key)) key_states[key] = StateInfo{ (State)(State::PRESSED | State::DOWN), modifiers };
        else key_states[key] = StateInfo{ State::DOWN, modifiers };
    }
}

void PTInput::handleMouseEvent(int button, int action, int mods)
{
    Modifiers modifiers = Modifiers::NONE;
    if (mods & GLFW_MOD_SHIFT)
        modifiers = (Modifiers)(modifiers | Modifiers::SHIFT);
    if (mods & GLFW_MOD_CONTROL)
        modifiers = (Modifiers)(modifiers | Modifiers::CTRL);
    if (mods & GLFW_MOD_ALT)
        modifiers = (Modifiers)(modifiers | Modifiers::ALT);

    MouseButton but = MouseButton::MOUSE_NONE;
    switch (button)
    {
        case GLFW_MOUSE_BUTTON_LEFT: but = MOUSE_LEFT; break;
        case GLFW_MOUSE_BUTTON_RIGHT: but = MOUSE_RIGHT; break;
        case GLFW_MOUSE_BUTTON_MIDDLE: but = MOUSE_MIDDLE; break;
        default: but = MOUSE_NONE; break;
    }

    if (action == GLFW_RELEASE)
        mouse_states[but] = StateInfo{ State::RELEASED, Modifiers::NONE };
    else if (action == GLFW_PRESS)
        mouse_states[but] = StateInfo{ (State)(State::PRESSED | State::DOWN), modifiers };
    else if (action == GLFW_REPEAT)
    {
        if (wasMousePressed(but)) mouse_states[but] = StateInfo{ (State)(State::PRESSED | State::DOWN), modifiers };
        else mouse_states[but] = StateInfo{ State::DOWN, modifiers };
    }
}

bool PTInput::isKeyDown(int key) const
{
    auto it = key_states.find(key);
    if (it != key_states.end())
        return (*it).second.state & State::DOWN;
    
    return false;
}

bool PTInput::wasKeyPressed(int key)
{
    auto it = key_states.find(key);
    if (it != key_states.end())
    {
        bool pressed = (*it).second.state & State::PRESSED;
        if (pressed)
            (*it).second.state = State::DOWN;
        return pressed;
    }
    
    return false;
}

bool PTInput::wasKeyReleased(int key)
{
    auto it = key_states.find(key);
    if (it != key_states.end())
    {
        bool released = (*it).second.state & State::RELEASED;
        if (released)
            (*it).second.state = State::UP;
        return released;
    }
    
    return false;
}

PTInput::Modifiers PTInput::getKeyModifiers(int key) const
{
    auto it = key_states.find(key);
    if (it != key_states.end())
    {
        return (*it).second.modifiers;
    }
    
    return Modifiers::NONE;
}

bool PTInput::isMouseDown(MouseButton button) const
{
    auto it = mouse_states.find(button);
    if (it != mouse_states.end())
        return (*it).second.state & State::DOWN;
    
    return false;
}

bool PTInput::wasMousePressed(MouseButton button)
{
    auto it = mouse_states.find(button);
    if (it != mouse_states.end())
    {
        bool pressed = (*it).second.state & State::PRESSED;
        if (pressed)
            (*it).second.state = State::DOWN;
        return pressed;
    }
    
    return false;
}

bool PTInput::wasMouseReleased(MouseButton button)
{
    auto it = mouse_states.find(button);
    if (it != mouse_states.end())
    {
        bool released = (*it).second.state & State::RELEASED;
        if (released)
            (*it).second.state = State::UP;
        return released;
    }
    
    return false;
}

PTVector2i PTInput::getMousePosition() const
{
    return mouse_position;
}

void PTInput::setMousePosition(PTVector2i position)
{
    mouse_position = position;
    glfwSetCursorPos(window, static_cast<double>(position.x), static_cast<double>(position.y));
}

void PTInput::setMouseVisible(bool visible)
{
    if (visible) glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    else glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
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

void PTInput::keyboardCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (input_manager) input_manager->handleKeyboardEvent(key, action, mods);
}

void PTInput::cursorCallback(GLFWwindow* window, double xpos, double ypos)
{
    if (input_manager) input_manager->mouse_position = PTVector2i{ static_cast<int32_t>(xpos), static_cast<int32_t>(ypos) };
}

void PTInput::mouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    if (input_manager) input_manager->handleMouseEvent(button, action, mods);
}

PTInput::PTInput(GLFWwindow* _window)
{
    window = _window;
    glfwSetKeyCallback(window, keyboardCallback);
    glfwSetCursorPosCallback(window, cursorCallback);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);

    for (uint8_t i = 0; i < 4; i++)
        gamepads[i] = PTGamepad(i);

    should_exit = false;
    mainloop_thread = thread([this] { this->mainLoop(); });
}

PTInput::~PTInput()
{
    should_exit = true;
    
    mainloop_thread.join();
}

void PTInput::mainLoop()
{
    while (!should_exit)
    {
        debugLog("poll!");
        glfwPollEvents();
        pollGamepads();
    }
}