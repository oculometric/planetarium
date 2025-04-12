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
    
    enum Modifiers
    {
        NONE        = 0b0000,
        SHIFT       = 0b0001,
        CTRL        = 0b0010,
        ALT         = 0b0100
    };

    enum MouseButton
    {
        MOUSE_NONE      = 0b0000,
        MOUSE_LEFT      = 0b0001,
        MOUSE_RIGHT     = 0b0010,
        MOUSE_MIDDLE    = 0b0100
    };

private:
    enum State
    {
        UP          = 0b0000,
        DOWN        = 0b0001,
        PRESSED     = 0b0010,
        RELEASED    = 0b0100
    };

    struct StateInfo
    {
        State state = UP;
        Modifiers modifiers = NONE;
    };

private:
    std::array<PTGamepad, 4> gamepads;
    std::map<int, StateInfo> key_states;
    std::map<MouseButton, StateInfo> mouse_states;
    PTVector2i mouse_position;

    GLFWwindow* window;

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

    bool isMouseDown(MouseButton button) const;
    bool wasMousePressed(MouseButton button);
    bool wasMouseReleased(MouseButton button);

    PTVector2i getMousePosition() const;
    void setMousePosition(PTVector2i position);
    void setMouseVisible(bool visible);

    // TODO: scroll wheel detection

    PTInput(PTInput& other) = delete;
    PTInput(PTInput&& other) = delete;
    PTInput operator=(PTInput& other) = delete;
    PTInput operator=(PTInput&& other) = delete;

private:
    PTInput(GLFWwindow* _window);
    ~PTInput();

    static void keyboardCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void cursorCallback(GLFWwindow* window, double xpos, double ypos);
    static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
    void handleKeyboardEvent(int key, int action, int mods);
    void handleMouseEvent(int button, int action, int mods);
    void pollGamepads();

    void mainLoop();
};