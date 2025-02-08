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
layout(location = 5) out vec3 frag_wp;
layout(location = 6) out vec3 frag_wn;

const vec4 snap_resolution = vec4(640, 480, 200, 10000) / 8.0f;

void main()
{
    frag_position = in_position;
    frag_wp = (transform.model_to_world * vec4(in_position, 1.0)).xyz;
    frag_wn = (normalize(transform.model_to_world * vec4(in_normal, 0.0))).xyz;
    gl_Position = transform.world_to_clip * vec4(frag_wp, 1.0f);
    //gl_Position = floor(gl_Position * snap_resolution) / snap_resolution;
    frag_colour = in_color;
    frag_normal = in_normal;
    frag_tangent = in_tangent;
    frag_uv = in_uv;
}