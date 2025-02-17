#include "input/gamepad.h"

#include <fcntl.h>
#include <string>
#ifdef _WIN32
#else
#include <unistd.h>
#include <linux/joystick.h>
#include <pthread.h>
#endif

#include "debug.h"
#include "input/gamepad.h"

using namespace std;

PTGamepad::PTGamepad(uint8_t index)
{
#ifdef _WIN32
#else
    device_handle = open((string("/dev/input/js") + to_string(index)).c_str(), O_RDONLY | O_NONBLOCK);
#endif
}

bool PTGamepad::getButtonState(Button button) const
{
    return button_states & button;
}

PTVector2f PTGamepad::getJoystickState(Axis axis) const
{
    switch (axis)
    {
    case LEFT_AXIS: return left_axis;
    case RIGHT_AXIS: return right_axis;
    }

    return PTVector2f{ 0,0 };
}

PTGamepad::Event PTGamepad::poll()
{
    Event event{ };

    if (!isValid()) return event;

#ifdef _WIN32
#else
    js_event raw_event{ };

    ssize_t bytes_read = read(device_handle, &raw_event, sizeof(raw_event));

    if (bytes_read != sizeof(raw_event)) return event;

    event.is_valid = true;
    event.type = (EventType)(raw_event.type);
    // FIXME: joystick events just never registering now, apart from the invalid 117 event (what?)
    event.element_index = raw_event.number;
    event.value = raw_event.value;

#endif

    if (event.type == EventType::BUTTON)
    {
        switch (event.element_index)
        {
        case 0: event.element_index = Button::CONTROL_SOUTH; break;
        case 1: event.element_index = Button::CONTROL_EAST; break;
        case 2: event.element_index = Button::CONTROL_WEST; break;
        case 3: event.element_index = Button::CONTROL_NORTH; break;
        case 4: event.element_index = Button::LEFT_MINOR; break;
        case 5: event.element_index = Button::RIGHT_MINOR; break;
        case 6: event.element_index = Button::MENU; break;
        case 7: event.element_index = Button::START; break;
        case 9: event.element_index = Button::LEFT_STICK; break;
        case 10: event.element_index = Button::RIGHT_STICK; break;
        default:
            return event;
        }
        setButtonState((Button)(event.element_index), event.value); 
    }
    else if (event.type == EventType::JOYSTICK)
    {
        switch (event.element_index)
        {
        case 0: setJoystickState((Axis)(event.element_index = Axis::LEFT_AXIS), (float)event.value / (float)INT16_MAX, left_axis.y); break;
        case 1: setJoystickState((Axis)(event.element_index = Axis::LEFT_AXIS), left_axis.x, (float)event.value / (float)INT16_MAX); break;
        case 2: setButtonState((Button)(event.element_index = Button::LEFT_MAJOR), event.value > 0); break;
        case 3: setJoystickState((Axis)(event.element_index = Axis::RIGHT_AXIS), (float)event.value / (float)INT16_MAX, right_axis.y); break;
        case 4: setJoystickState((Axis)(event.element_index = Axis::RIGHT_AXIS), right_axis.x, (float)event.value / (float)INT16_MAX); break;
        case 5: setButtonState((Button)(event.element_index = Button::RIGHT_MAJOR), event.value > 0); break;
        case 6: setButtonState((Button)(event.element_index = Button::DIRECTIONAL_LEFT), event.value < 0);
                setButtonState((Button)(event.element_index = Button::DIRECTIONAL_RIGHT), event.value > 0);
                break;
        case 7: setButtonState((Button)(event.element_index = Button::DIRECTIONAL_UP), event.value < 0);
                setButtonState((Button)(event.element_index = Button::DIRECTIONAL_LEFT), event.value > 0);
                break;
        }
    }
    else if (event.type == EventType::INIT)
        debugLog("gamepad " + to_string(device_handle) + " initialised.");
    else
        debugLog("uh oh! invalid gamepad event: " + to_string(event.type));

    return event;
}

bool PTGamepad::isValid()
{
    return device_handle != -1;
}

PTGamepad::~PTGamepad()
{
    if (isValid())
        close(device_handle);
}

void PTGamepad::setButtonState(Button button, bool state)
{
    button_states &= ~button;
    if (state) button_states |= button;
}

void PTGamepad::setJoystickState(Axis axis, float x, float y)
{
    switch (axis)
    {
    case Axis::LEFT_AXIS: left_axis = { x, y }; break;
    case Axis::RIGHT_AXIS: right_axis = { x, y }; break;
    }
}