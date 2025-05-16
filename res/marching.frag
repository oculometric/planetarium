#version 450

#extension GL_GOOGLE_include_directive : enable
#include "engine/shader/common.glsl"

UNIFORM_TRANSFORM
UNIFORM_SCENE

VARYING_COMMON(in)
FRAGMENT_OUTPUTS

void main()
{
    frag_colour = vec4(1);
    frag_normal = normalize(varyings.world_normal);
}