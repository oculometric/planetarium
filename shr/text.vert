#version 450

#include "common.glsl"

VERTEX_INPUTS
UNIFORM_COMMON

// TODO: text character array (buffer?)
// TODO: font texture

VARYING_COMMON(out)

void main()
{
    STANDARD_VARYING_MATH
}