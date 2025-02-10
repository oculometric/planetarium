#version 450

#include "common.glsl"

UNIFORM_COMMON
VARYING_COMMON(in)
FRAGMENT_OUTPUTS

const vec3 sun_direction = normalize(vec3(0.1, 0.4, -0.9)); // TODO: this would be moved into the common uniforms

const float divs = 6.0;
const float pixel_size = 1.0f;

void main()
{
    float brightness = clamp(dot(-sun_direction, varyings.world_normal), 0, 1);

    float scaled = brightness * divs;
    brightness = (floor(scaled) + float(fract(scaled) > ((float(int(gl_FragCoord.x / pixel_size) % 2 == int(gl_FragCoord.y / pixel_size) % 2) + 1.0) / 3.0))) / divs;

    frag_colour = vec4(vec3(brightness), 1);
}