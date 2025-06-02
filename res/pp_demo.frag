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
    vec3 c_sharp = (5.0f * c_center) - ((c_up + c_down + c_left + c_right));

    float blend = floor((varyings.uv.x * 4.0f) + (varyings.uv.y * 0.5f) - 0.5f);
    if (blend < 1.0f)
        frag_colour = vec4(texture(albedo_texture, varyings.uv).rgb, 1.0f);
    else if (blend < 2.0f)
        frag_colour = vec4(vec3(pow(texture(depth_texture, varyings.uv).r, 3.0f)), 1.0f);
    else if (blend < 3.0f)
        frag_colour = vec4(texture(normal_texture, varyings.uv).rgb, 1.0f);
    else
        frag_colour = vec4(texture(extra_texture, varyings.uv).rgb, 1.0f);
}