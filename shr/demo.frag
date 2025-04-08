#version 450

#include "common.glsl"

UNIFORM_TRANSFORM
UNIFORM_SCENE

layout(binding = UNIFORM_OFFSET + 0) uniform MaterialProperties
{
    vec3 colour;
} properties;

VARYING_COMMON(in)
FRAGMENT_OUTPUTS

const vec3 sun_direction = normalize(vec3(0.1, 0.4, -0.9)); // TODO: this would be moved into the common uniforms

const float divs = 6.0;
const float pixel_size = 1.0f;

void main()
{
    float brightness = clamp(dot(-sun_direction, varyings.world_normal), 0, 1);
    vec3 surface = (properties.colour + 0.1f) * brightness;

    vec3 scaled = surface * divs;
    float val = (float(int(gl_FragCoord.x / pixel_size) % 2 == int(gl_FragCoord.y / pixel_size) % 2) + 1.0) / 3.0;
    surface = (floor(scaled) + vec3(greaterThan(fract(scaled), vec3(val)))) / divs;

    frag_colour = vec4(vec3(surface), 1);
    frag_normal = normalize(varyings.world_normal);
}