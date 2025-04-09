#pragma once

#include <stdint.h>
#include <array>
#include <GLFW/glfw3.h>
#include <map>

#include "gamepad.h"

struct PTKeyState
{
    int key;
    int action; // TODO: implement a system for detecting if key down on this frame
    int modifiers;
};

class PTInput
{
private:
    std::array<PTGamepad, 4> gamepads;
    std::map<int, PTKeyState> key_states;

public:
    static void init();
    static void deinit();
    static PTInput* get();

    bool getButtonState(PTGamepad::Button button, uint8_t gamepad) const;
    PTVector2f getJoystickState(PTGamepad::Axis axis, uint8_t gamepad) const;

    bool getButtonState(PTGamepad::Button button) const;
    PTVector2f getJoystickState(PTGamepad::Axis axis) const;

    void handleKeyboardEvent(int key, int action, int mods);
    
    PTKeyState getKeyState(int key) const;

    PTInput(PTInput& other) = delete;
    PTInput(PTInput&& other) = delete;
    PTInput operator=(PTInput& other) = delete;
    PTInput operator=(PTInput&& other) = delete;

    void pollGamepads(); // TODO: make input run on its own thread

private:
    PTInput();

    void translate(int key, int action, int mods);
};

void keyboardCallback(GLFWwindow* window, int key, int scancode, int action, int mods);