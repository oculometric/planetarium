#version 450

#extension GL_GOOGLE_include_directive : enable
#include "common.glsl"

UNIFORM_TRANSFORM
UNIFORM_SCENE

layout(binding = UNIFORM_OFFSET + 0) uniform TextBuffer
{
    uint[256] text;
} text_buffer;

VARYING_COMMON(in)

FRAGMENT_OUTPUTS

const vec3[8] colours = 
{
    vec3( 0.0f, 0.0f, 0.0f ),
    vec3( 1.0f, 0.0f, 0.0f ),
    vec3( 0.0f, 1.0f, 0.0f ),
    vec3( 0.0f, 0.0f, 1.0f ),
    vec3( 1.0f, 0.0f, 1.0f ),
    vec3( 1.0f, 1.0f, 0.0f ),
    vec3( 0.0f, 1.0f, 1.0f ),
    vec3( 1.0f, 1.0f, 1.0f )
};

void main()
{
    uvec2 uuv = uvec2(floor(varyings.uv * 16));
    frag_colour = vec4(colours[text_buffer.text[uuv.x + (16 * uuv.y)] % 8], 1.0f);
    frag_normal = varyings.normal;
}