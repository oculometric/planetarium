#version 450

#extension GL_GOOGLE_include_directive : enable
#include "common.glsl"

UNIFORM_TRANSFORM
UNIFORM_SCENE

layout(binding = UNIFORM_OFFSET + 0) uniform TextBuffer
{
    uvec4[64] text;
} text_buffer;

VARYING_COMMON(in)

FRAGMENT_OUTPUTS

const vec3[16] colours = 
{
    vec3( 0.0f, 0.0f, 0.0f ),
    vec3( 1.0f, 0.0f, 0.0f ),
    vec3( 0.0f, 1.0f, 0.0f ),
    vec3( 0.0f, 0.0f, 1.0f ),
    vec3( 1.0f, 0.0f, 1.0f ),
    vec3( 1.0f, 1.0f, 0.0f ),
    vec3( 0.0f, 1.0f, 1.0f ),
    vec3( 1.0f, 1.0f, 1.0f ),

    vec3( 0.5f, 0.0f, 0.0f ),
    vec3( 0.5f, 0.5f, 0.5f ),
    vec3( 1.0f, 0.5f, 0.0f ),
    vec3( 0.5f, 1.0f, 0.0f ),
    vec3( 0.5f, 0.5f, 0.5f ),
    vec3( 1.0f, 0.5f, 0.5f ),
    vec3( 0.5f, 1.0f, 0.5f ),
    vec3( 0.5f, 0.5f, 1.0f ),
    //vec3( 1.0f, 0.5f, 1.0f ),
    //vec3( 1.0)
};

// buffer size is 64, which means total packed number of chars is 64 * 4 * 4 = 1024
// thus the canvas should be 32 x 32

void main()
{
    // grid_uv is the character index in the 32x32 character grid
    uvec2 grid_uv = uvec2(floor(varyings.uv * 32));
    grid_uv.y = 31 - grid_uv.y;
    // index is the index of the character into the buffer
    uint index = grid_uv.x + (32 * grid_uv.y);
    // packed16 is the packed set of 16 characters
    uvec4 packed16 = (text_buffer.text[index / 16]);
    // packed4 is the packed set of 4 characters
    uint packed4 = packed16[(index / 4) % 4];
    uint value = (packed4 >> (8 * (index % 4))) % 256;

    frag_colour = vec4(colours[value % 16], 1.0f);
    frag_normal = varyings.normal;
}