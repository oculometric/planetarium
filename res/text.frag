#version 450

#extension GL_GOOGLE_include_directive : enable
#include "engine/shader/common.glsl"

UNIFORM_TRANSFORM
UNIFORM_SCENE

layout(binding = UNIFORM_OFFSET + 0) uniform TextBuffer
{
    uvec4[64] text;
} text_buffer;

// font texture should be 16x16 characters
layout(binding = UNIFORM_OFFSET + 1) uniform sampler2D font_texture;

VARYING_COMMON(in)

FRAGMENT_OUTPUTS

// buffer size is 64, which means total packed number of chars is 64 * 4 * 4 = 1024
// thus the canvas should be 32 x 32

vec2 flipUV(vec2 uv)
{
    return vec2(uv.x, 1.0f - uv.y);
}

void main()
{
    vec2 font_aspect_ratio = vec2(1.0f, 9.0f / 7.0f);
    
    // grid_uv is the character index in the 32x32 character grid
    uvec2 grid_uv = uvec2(floor(flipUV(varyings.uv) * 32.0f / font_aspect_ratio));
    // index is the index of the character into the buffer
    uint index = grid_uv.x + (32 * grid_uv.y);
    // packed16 is the packed set of 16 characters
    uvec4 packed16 = (text_buffer.text[index / 16]);
    // packed4 is the packed set of 4 characters
    uint packed4 = packed16[(index / 4) % 4];
    uint char = (packed4 >> (8 * (index % 4))) % 256;

    // calculate a UV base offset into the font texture
    uint font_x = char % 16;
    uint font_y = char / 16;
    vec2 font_uv_base = vec2(font_x / 16.0f, 1.0f - (font_y / 16.0f));
    vec2 font_tile_uv = flipUV(fract(flipUV(varyings.uv) * 32.0f / font_aspect_ratio));

    frag_colour = vec4(texture(font_texture, font_uv_base + (font_tile_uv / 16.0f)).rgb, 1.0f);
    //frag_colour = vec4(font_tile_uv, 0.0f, 1.0f);
    //frag_colour = vec4(colours[value % 16], 1.0f);
    //frag_colour = texture(font_texture, varyings.uv);
    frag_normal = varyings.normal;
}