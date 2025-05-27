#version 450

#extension GL_GOOGLE_include_directive : enable
#include "common.glsl"

UNIFORM_TRANSFORM
UNIFORM_SCENE

layout(binding = UNIFORM_OFFSET + 0) uniform MaterialProperties
{
    vec3 colour;
} properties;

VARYING_COMMON(in)
FRAGMENT_OUTPUTS

const float divs = 6.0;
const float pixel_size = 1.0f;

void main()
{
    vec3 surface_colour = properties.colour;
    vec3 surface_lit = vec3(0);

    for (int i = 0; i < 16; i++)
    {
        LightDescription light = scene.lights[i];
        float light_dot = 1.0f;
        vec3 light_dir = light.direction;
        if (light.is_directional < 0.5)
        {
            vec3 offset = varyings.world_position - light.position;
            light_dir = normalize(offset);
            float dot_dir = dot(light.direction, light_dir);
            if (dot_dir < light.cos_half_ang_radians)
                light_dot = 0;
            
            light_dot *= 1.0f / (dot(offset, offset) + 0.01f);
        }
        light_dot *= clamp(dot(-light_dir, varyings.world_normal), 0, 1);

        surface_lit += surface_colour * light.colour * light_dot * light.multiplier;
    }
    
    //vec3 scaled = surface * divs;
    //float val = (float(int(gl_FragCoord.x / pixel_size) % 2 == int(gl_FragCoord.y / pixel_size) % 2) + 1.0) / 3.0;
    //surface = (floor(scaled) + vec3(greaterThan(fract(scaled), vec3(val)))) / divs;

    frag_colour = vec4(surface_lit, 1);
    frag_normal = normalize(varyings.world_normal);
}