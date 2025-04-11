#pragma once

#include <stdint.h>
#include <array>
#include <map>
#include <thread>

#include "gamepad.h"

struct GLFWwindow;

class PTInput
{
public:
    enum State
    {
        UP          = 0b0000,
        DOWN        = 0b0001,
        PRESSED     = 0b0010,
        RELEASED    = 0b0100
    };

    enum Modifiers
    {
        NONE        = 0b0000,
        SHIFT       = 0b0001,
        CTRL        = 0b0010,
        ALT         = 0b0100
    };

    struct KeyInfo
    {
        State state;
        Modifiers modifiers;
    };

private:
    std::array<PTGamepad, 4> gamepads;
    std::map<int, KeyInfo> key_states;

    std::thread mainloop_thread;
    bool should_exit = false;

public:
    static void init(GLFWwindow* window);
    static void deinit();
    static PTInput* get();

    bool getButtonState(PTGamepad::Button button, uint8_t gamepad) const;
    PTVector2f getJoystickState(PTGamepad::Axis axis, uint8_t gamepad) const;

    bool getButtonState(PTGamepad::Button button) const;
    PTVector2f getJoystickState(PTGamepad::Axis axis) const;
    
    bool isKeyDown(int key) const;
    bool wasKeyPressed(int key);
    bool wasKeyReleased(int key);
    Modifiers getKeyModifiers(int key) const;

    PTInput(PTInput& other) = delete;
    PTInput(PTInput&& other) = delete;
    PTInput operator=(PTInput& other) = delete;
    PTInput operator=(PTInput&& other) = delete;

private:
    PTInput(GLFWwindow* window);
    ~PTInput();

    static void keyboardCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
    void handleKeyboardEvent(int key, int action, int mods);
    void pollGamepads();

    void mainLoop();
};