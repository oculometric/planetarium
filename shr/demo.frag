#version 450

layout(location = 0) out vec4 out_colour;
layout(location = 0) in vec3 frag_colour;
layout(location = 1) in vec3 frag_position;

const mat4 bayer =
{
    { 0, 8,  2, 10 },
    { 4, 12, 6, 14 },
    { 1, 9,  3, 11 },
    { 5, 13, 7, 15 }
};

float filter_value(vec2 screen_position, vec2 grid_divisions)
{
    ivec2 grid_position = ivec2(floor(screen_position * grid_divisions));
    return bayer[grid_position.x % 4][grid_position.y % 4] / 16.0;
}

const vec2 screen_size = vec2(640.0, 480.0);
const float pixels_size = 4.0;
const float divisions = 2.0;

void main()
{
    vec3 inp = frag_colour * divisions;

    vec3 outp = (
        floor(inp) + vec3(greaterThan(
            fract(inp),
            vec3(filter_value(frag_position.xy / 2, screen_size / pixels_size))
        ))
    ) / divisions;
    
    out_colour = vec4(vec3(outp), 1.0);
}