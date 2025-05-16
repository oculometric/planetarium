#version 450

#extension GL_GOOGLE_include_directive : enable
#include "engine/shader/common.glsl"

VERTEX_INPUTS

UNIFORM_TRANSFORM
UNIFORM_SCENE

VARYING_COMMON(out)

void main()
{
    STANDARD_VARYING_MATH
}