#pragma once

#include <stdint.h>
#ifdef _WIN32
#else
#include <linux/joystick.h>
#endif

#include "math/vector2.h"

// TODO: make this platform-independent (or at least, reimplement it for all platforms)

class PTGamepad
{
public:
    enum EventType
    {
    #ifdef _WIN32
        INVALID = 0,
        BUTTON,
        JOYSTICK,
        INIT
    #else
        INVALID     = 0b00000000,
        BUTTON      = JS_EVENT_BUTTON,
        JOYSTICK    = JS_EVENT_AXIS,
        INIT        = JS_EVENT_INIT
    #endif
    };

    struct Event
    {
        bool is_valid = false;
        EventType type = EventType::INVALID;
        uint32_t element_index = 0;
        int16_t value = 0;
    };

    enum Button
    {
        DIRECTIONAL_UP      = 0b0000000000000001,
        DIRECTIONAL_DOWN    = 0b0000000000000010,
        DIRECTIONAL_LEFT    = 0b0000000000000100,
        DIRECTIONAL_RIGHT   = 0b0000000000001000,
        CONTROL_NORTH       = 0b0000000000010000,
        CONTROL_SOUTH       = 0b0000000000100000,
        CONTROL_WEST        = 0b0000000001000000,
        CONTROL_EAST        = 0b0000000010000000,
        RIGHT_MINOR         = 0b0000000100000000,
        RIGHT_MAJOR         = 0b0000001000000000,
        LEFT_MINOR          = 0b0000010000000000,
        LEFT_MAJOR          = 0b0000100000000000,
        LEFT_STICK          = 0b0001000000000000,
        RIGHT_STICK         = 0b0010000000000000,
        MENU                = 0b0100000000000000,
        START               = 0b1000000000000000
    };

    enum Axis
    {
        LEFT_AXIS,
        RIGHT_AXIS
    };

private:
    int device_handle = -1;

    uint32_t button_states = 0;
    PTVector2f left_axis = { 0.0f, 0.0f };
    PTVector2f right_axis = { 0.0f, 0.0f };

public:
    inline PTGamepad() : PTGamepad(255) { }
    PTGamepad(uint8_t index);

    bool getButtonState(Button button) const;
    PTVector2f getJoystickState(Axis axis) const;

    Event poll();
    bool isValid();

    ~PTGamepad();

private:
    void setButtonState(Button button, bool state);
    void setJoystickState(Axis axis, float x, float y);
};