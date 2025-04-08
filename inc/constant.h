#pragma once

#include "math/ptmath.h"

const uint32_t MAX_FRAMES_IN_FLIGHT = 2;

const uint16_t TRANSFORM_UNIFORM_BINDING = 0;
const uint16_t SCENE_UNIFORM_BINDING = 1;

struct TransformUniforms
{
    float model_to_world[16];
    float world_to_view[16];
    float view_to_clip[16];
    uint32_t object_id;
};

struct SceneUniforms
{
    PTVector2f viewport_size;
    float time;
};