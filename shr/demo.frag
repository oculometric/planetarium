#version 450

layout(binding = 0) uniform TransformMatrices
{
    mat4 model_to_world;
    mat4 world_to_clip;
} transform;

layout(location = 0) in vec3 frag_colour;
layout(location = 1) in vec3 frag_position;
layout(location = 2) in vec3 frag_normal;
layout(location = 3) in vec3 frag_tangent;
layout(location = 4) in vec2 frag_uv;
layout(location = 5) in vec3 frag_wp;
layout(location = 6) in vec3 frag_wn;

layout(location = 0) out vec4 out_colour;

const vec3 sun_direction = normalize(vec3(0.1, 0.2, -0.9));

void main()
{
    float brightness = clamp(dot(-sun_direction, frag_wn), 0, 1);
    out_colour = vec4(vec3(brightness), 1);
}