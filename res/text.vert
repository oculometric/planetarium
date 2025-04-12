#version 450

#include "engine/shader/common.glsl"

VERTEX_INPUTS

UNIFORM_TRANSFORM
UNIFORM_SCENE

VARYING_COMMON(out)

void main()
{
    STANDARD_VARYING_MATH
}