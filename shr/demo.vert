#version 450

layout(binding = 0) uniform TransformMatrices
{
    mat4 model_to_world;
    mat4 world_to_clip;
} transform;

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_color;
layout(location = 2) in vec3 in_normal;
layout(location = 3) in vec3 in_tangent;
layout(location = 4) in vec2 in_uv;

layout(location = 0) out vec3 frag_colour;
layout(location = 1) out vec3 frag_position;
layout(location = 2) out vec3 frag_normal;
layout(location = 3) out vec3 frag_tangent;
layout(location = 4) out vec2 frag_uv;

void main()
{
    gl_Position = transform.world_to_clip * transform.model_to_world * vec4(in_position, 1.0);
    frag_position = in_position;
    frag_colour = in_color;
    frag_normal = in_normal;
    frag_tangent = in_tangent;
    frag_uv = in_uv;
}