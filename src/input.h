#pragma once

#include <stdint.h>

// TODO: make this platform-independent (or at least, reimplement it for all platforms)

enum PTControllerEventType
{
    INVALID     = 0b00000000,
    BUTTON_DOWN = 0b00000001,
    BUTTON_UP   = 0b00000010,
    JOYSTICK    = 0b00000100,
    INIT        = 0b00001000
};

struct PTControllerEvent
{
    bool is_valid = false;
    PTControllerEventType type = PTControllerEventType::INVALID;
    uint8_t element_index = 0;
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