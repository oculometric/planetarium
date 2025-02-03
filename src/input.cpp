#include "input.h"

#include <fcntl.h>
#include <string>
#include <unistd.h>
#include <linux/joystick.h>
#include <iostream>
#include <pthread.h>

#include "debug.h"

using namespace std;

PTController::PTController()
{
    PTController(0);
}

PTController::PTController(uint8_t index)
{
    device_handle = open((string("/dev/input/js") + to_string(index)).c_str(), O_RDONLY | O_NONBLOCK);
}

PTControllerEvent PTController::poll()
{
    PTControllerEvent event{ };

    if (!isValid()) return event;

    js_event raw_event{ };

    ssize_t bytes_read = read(device_handle, &raw_event, sizeof(raw_event));

    if (bytes_read != sizeof(raw_event)) return event;

    event.is_valid = true;
    event.element_index = raw_event.number;
    event.value = raw_event.value;

    switch(raw_event.type)
    {
    case JS_EVENT_AXIS: event.type = PTControllerEventType::JOYSTICK; break;
    case JS_EVENT_BUTTON: event.type = PTControllerEventType::BUTTON; break;
    case JS_EVENT_INIT: event.type = PTControllerEventType::INIT; break;
    default:
        event.type = (PTControllerEventType)((int)(raw_event.type) << 8); break;
    }

    return event;
}

bool PTController::isValid()
{
    return device_handle != -1;
}

PTInputManager* input_manager = nullptr;

void initInputManager(PTInputManager* manager)
{
    input_manager = manager;
}

void keyboardCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (input_manager) input_manager->handleKeyboardEvent(key, action, mods);
}

void PTInputManager::translate(PTControllerEvent gamepad_event)
{
    if (gamepad_event.type == PTControllerEventType::BUTTON)
    {
        switch (gamepad_event.element_index)
        {
        case 0: setButtonState(PTInputButton::CONTROL_SOUTH, gamepad_event.value); break;
        case 1: setButtonState(PTInputButton::CONTROL_EAST, gamepad_event.value); break;
        case 2: setButtonState(PTInputButton::CONTROL_WEST, gamepad_event.value); break;
        case 3: setButtonState(PTInputButton::CONTROL_NORTH, gamepad_event.value); break;
        case 4: setButtonState(PTInputButton::LEFT_MINOR, gamepad_event.value); break;
        case 5: setButtonState(PTInputButton::RIGHT_MINOR, gamepad_event.value); break;
        case 6: setButtonState(PTInputButton::MENU, gamepad_event.value); break;
        case 7: setButtonState(PTInputButton::START, gamepad_event.value); break;
        case 9: setButtonState(PTInputButton::MOVE_AXIS, gamepad_event.value); break;
        case 10: setButtonState(PTInputButton::LOOK_AXIS, gamepad_event.value); break;
        }
    }
    else if (gamepad_event.type == PTControllerEventType::JOYSTICK)
    {
        switch (gamepad_event.element_index)
        {
        case 0: setAxisState(PTInputAxis::MOVE_AXIS_X, gamepad_event.value); break;
        case 1: setAxisState(PTInputAxis::MOVE_AXIS_Y, gamepad_event.value); break;
        case 2: setButtonState(PTInputButton::LEFT_MAJOR, gamepad_event.value > 0); break;
        case 3: setAxisState(PTInputAxis::LOOK_AXIS_X, gamepad_event.value); break;
        case 4: setAxisState(PTInputAxis::LOOK_AXIS_Y, gamepad_event.value); break;
        case 5: setButtonState(PTInputButton::RIGHT_MAJOR, gamepad_event.value > 0); break;
        }
    }
    else if (gamepad_event.type == PTControllerEventType::INIT)
    {
        debugLog("gamepad init");
    }
    else
    {
        debugLog("uh oh! invalid gamepad event: " + to_string(gamepad_event.type >> 8));
    }
}

void PTInputManager::translate(int key, int action, int mods)
{
    if (action == 2) return;
    
    debugLog("keyboard event: key (" + to_string(key) + "), action (" + to_string(action) + "), mods (" + to_string(mods) + ")");
    key_states[key] = PTKeyState{ key, action, mods };
}

void PTInputManager::setButtonState(PTInputButton button, bool state)
{
    button_states &= ~button;
    if (state) button_states |= button;
}

void PTInputManager::setAxisState(PTInputAxis axis, int16_t value)
{
    switch (axis)
    {
    case PTInputAxis::LOOK_AXIS_X: look_axis_x = value; break;
    case PTInputAxis::LOOK_AXIS_Y: look_axis_y = value; break;
    case PTInputAxis::MOVE_AXIS_X: move_axis_x = value; break;
    case PTInputAxis::MOVE_AXIS_Y: move_axis_y = value; break;
    }
}

void PTInputManager::pollControllers()
{
    PTControllerEvent event{ };
    if (gamepad_0.isValid())
    {
        while (true)
        {
            event = gamepad_0.poll();
            if (!event.is_valid) break;
            translate(event);
        }
    }
    if (gamepad_1.isValid())
    {
        while (true)
        {
            event = gamepad_1.poll();
            if (!event.is_valid) break;
            translate(event);
        }
    }
    if (gamepad_2.isValid())
    {
        while (true)
        {
            event = gamepad_2.poll();
            if (!event.is_valid) break;
            translate(event);
        }
    }
    if (gamepad_3.isValid())
    {
        while (true)
        {
            event = gamepad_3.poll();
            if (!event.is_valid) break;
            translate(event);
        }
    }
}

void PTInputManager::handleKeyboardEvent(int key, int action, int mods)
{
    translate(key, action, mods);
}

bool PTInputManager::getButtonState(PTInputButton button)
{
    return button_states & button;
}

int16_t PTInputManager::getAxisState(PTInputAxis axis)
{
    switch (axis)
    {
    case PTInputAxis::LOOK_AXIS_X:
        return look_axis_x;
    case PTInputAxis::LOOK_AXIS_Y:
        return look_axis_y;
    case PTInputAxis::MOVE_AXIS_X:
        return move_axis_x;
    case PTInputAxis::MOVE_AXIS_Y:
        return move_axis_y;
    }

    return 0;
}

PTKeyState PTInputManager::getKeyState(int key)
{
    if (key_states.contains(key))
        return key_states[key];
    return PTKeyState{ key, 0, 0 };
}

PTInputManager::PTInputManager()
{ }
