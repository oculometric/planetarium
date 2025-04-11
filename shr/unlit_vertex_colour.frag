#version 450

#include "common.glsl"

UNIFORM_TRANSFORM
UNIFORM_SCENE

VARYING_COMMON(in)

FRAGMENT_OUTPUTS

void main()
{
    frag_colour = vec4(varyings.colour, 1.0f);
    frag_normal = normalize(varyings.world_normal);
}