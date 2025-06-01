#version 450

#extension GL_GOOGLE_include_directive : enable
#include "engine/shader/common.glsl"

VERTEX_INPUTS

UNIFORM_TRANSFORM
UNIFORM_SCENE

VARYING_COMMON(out)

void main()
{
    varyings.position = vert_position;
    varyings.colour = vert_colour;
    varyings.normal = vert_normal;
    varyings.tangent = vert_tangent;
    varyings.bitangent = cross(varyings.normal, varyings.tangent);
    varyings.uv = vert_uv;
    gl_Position = vec4(vert_position.xy, 0.0f, 1.0f);
}