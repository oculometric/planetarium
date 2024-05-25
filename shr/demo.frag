#version 450

layout(location = 0) out vec4 outColour;
layout(location = 0) in vec3 frag_colour;

void main()
{
    outColour = vec4(frag_colour, 1.0);
}