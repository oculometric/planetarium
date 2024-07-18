#version 450

layout(location = 0) out vec3 frag_colour;

vec2 positions[6] = vec2[]
(
    vec2( -0.707, -0.707 ),
    vec2( 0.0,     0.707 ),
    vec2( 0.707,  -0.707 ),
    vec2( 0,       0),
    vec2( 0,       1),
    vec2( 1,       0)
);

vec3 colours[3] = vec3[]
(
    vec3( 0.0, 0.0, 1.0 ),
    vec3( 0.0, 1.0, 0.0 ),
    vec3( 1.0, 0.0, 0.0 )
);

void main()
{
    gl_Position = vec4(positions[gl_VertexIndex], float(gl_VertexIndex) / 4.0, 1.0);
    frag_colour = colours[gl_VertexIndex % 3];
}