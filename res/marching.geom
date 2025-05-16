#version 450

#extension GL_GOOGLE_include_directive : enable
#include "engine/shader/common.glsl"


UNIFORM_TRANSFORM
UNIFORM_SCENE

layout(triangles) in;
layout(triangle_strip, max_vertices = 9) out;

layout(location = 0) in vec3 in_position[];
layout(location = 1) in vec3 in_colour[];
layout(location = 2) in vec3 in_normal[];
layout(location = 3) in vec3 in_tangent[];
layout(location = 4) in vec2 in_uv[];

layout(location = 0) out vec3 out_position[];
layout(location = 1) out vec3 out_colour[];
layout(location = 2) out vec3 out_normal[];
layout(location = 3) out vec3 out_tangent[];
layout(location = 4) out vec2 out_uv[];

void main()
{
    // this code is called for each piece of geometry
    // TODO: actual code
    //out_colour = vec4(1);
    //frag_normal = normalize(varyings.world_normal);
}