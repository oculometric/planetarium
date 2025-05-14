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

const vec2 fontmap_glyph_size = vec2(10, 18);
const vec2 fontmap_size_chars = vec2(320, 320) / fontmap_glyph_size;
const float font_aspect_ratio = fontmap_glyph_size.y / fontmap_glyph_size.x;

const uint characters_per_line = 12;

void main()
{
    float tb_width_chars = float(characters_per_line);
    float tb_height_chars = tb_width_chars / font_aspect_ratio;
    vec2 tile_uv = flipUV(varyings.uv) * vec2(tb_width_chars, tb_height_chars);
    vec2 grid_uv = floor(tile_uv);

    uint index = uint((grid_uv.x + (tb_width_chars * grid_uv.y)));
    // packed16 is the packed set of 16 characters
    uvec4 packed16 = (text_buffer.text[index / 16]);
    // packed4 is the packed set of 4 characters
    uint packed4 = packed16[(index / 4) % 4];
    uint char = (packed4 >> (8 * (index % 4))) % 256;

    float font_x = fract(char / fontmap_size_chars.x);
    float font_y = floor(char / fontmap_size_chars.x) / fontmap_size_chars.y;
    vec2 font_uv = (fract(tile_uv) / fontmap_size_chars) + vec2(font_x, font_y);

    frag_colour = vec4(texture(font_texture, flipUV(font_uv)).rgb, 1.0f);
    frag_normal = varyings.normal;

    //frag_colour = vec4(fract(tile_uv) / fontmap_size_chars, 0, 1);
}