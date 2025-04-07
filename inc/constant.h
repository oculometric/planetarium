#pragma once

#include "math/ptmath.h"

const uint32_t MAX_FRAMES_IN_FLIGHT = 2;

const uint16_t COMMON_UNIFORM_BINDING = 0;

struct CommonUniforms
{
    float model_to_world[16];
    float world_to_view[16];
    float view_to_clip[16];
    PTVector2f viewport_size;
};