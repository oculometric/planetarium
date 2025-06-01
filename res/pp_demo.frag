#version 450

#extension GL_GOOGLE_include_directive : enable
#include "engine/shader/common.glsl"

UNIFORM_TRANSFORM
UNIFORM_SCENE

layout(binding = UNIFORM_OFFSET + 0) uniform sampler2D albedo_texture;
layout(binding = UNIFORM_OFFSET + 1) uniform sampler2D depth_texture;
layout(binding = UNIFORM_OFFSET + 2) uniform sampler2D normal_texture;
layout(binding = UNIFORM_OFFSET + 3) uniform sampler2D extra_texture;

VARYING_COMMON(in)
FRAGMENT_OUTPUTS

void main()
{
    vec2 inv_size = 2.0f / scene.viewport_size;

    vec3 c_up = texture(albedo_texture, varyings.uv + (inv_size * vec2(0, -1))).rgb;
    vec3 c_down = texture(albedo_texture, varyings.uv + (inv_size * vec2(0, 1))).rgb;
    vec3 c_left = texture(albedo_texture, varyings.uv + (inv_size * vec2(-1, 0))).rgb;
    vec3 c_right = texture(albedo_texture, varyings.uv + (inv_size * vec2(1, 0))).rgb;
    vec3 c_center = texture(albedo_texture, varyings.uv).rgb;

    frag_colour = vec4((5.0f * c_center) - ((c_up + c_down + c_left + c_right)), 1);
}