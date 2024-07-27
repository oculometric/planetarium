#pragma once

#include <stdint.h>
#include <GLFW/glfw3.h>

// TODO: make this platform-independent (or at least, reimplement it for all platforms)

enum PTControllerEventType
{
    INVALID     = 0b00000000,
    BUTTON      = 0b00000001,
    JOYSTICK    = 0b00000010,
    INIT        = 0b00000100
};

struct PTControllerEvent
{
    bool is_valid = false;
    PTControllerEventType type = PTControllerEventType::INVALID;
    uint8_t element_index = 0;
    int16_t value = 0;
};

class PTController
{
private:
    int device_handle = -1;
public:
    PTController();
    PTController(uint8_t index);

    PTControllerEvent poll();
    bool isValid();
};

enum PTInputButton
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
    MOVE_AXIS           = 0b0001000000000000,
    LOOK_AXIS           = 0b0010000000000000,
    MENU                = 0b0100000000000000,
    START               = 0b1000000000000000
};

enum PTInputAxis
{
    MOVE_AXIS_X = 1,
    MOVE_AXIS_Y = 2,
    LOOK_AXIS_X = 3,
    LOOK_AXIS_Y = 4
};

// TODO: make the keyboard controls for this configurable?
class PTInputManager
{
private:
    PTController gamepad_0 = PTController(0);
    PTController gamepad_1 = PTController(1);
    PTController gamepad_2 = PTController(2);
    PTController gamepad_3 = PTController(3);


    int16_t move_axis_x = 0;
    int16_t move_axis_y = 0;
    int16_t look_axis_x = 0;
    int16_t look_axis_y = 0;

    uint32_t button_states;

public:
    void pollControllers();
    void handleKeyboardEvent(int key, int action, int mods);
    
    bool getButtonState(PTInputButton button);
    int16_t getAxisState(PTInputAxis axis);

    PTInputManager();

    PTInputManager(PTInputManager& other) = delete;
    PTInputManager(PTInputManager&& other) = delete;
    PTInputManager operator=(PTInputManager& other) = delete;
    PTInputManager operator=(PTInputManager&& other) = delete;

private:
    void translate(PTControllerEvent gamepad_event);
    void translate(int key, int action, int mods);

    void setButtonState(PTInputButton button, bool state);
    void setAxisState(PTInputAxis axis, int16_t value);
};

void initInputManager(PTInputManager* manager);

void keyboardCallback(GLFWwindow* window, int key, int scancode, int action, int mods);