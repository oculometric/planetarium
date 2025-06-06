#define VERTEX_INPUTS layout(location = 0) in vec3 vert_position; \
layout(location = 1) in vec3 vert_colour; \
layout(location = 2) in vec3 vert_normal; \
layout(location = 3) in vec3 vert_tangent; \
layout(location = 4) in vec2 vert_uv;

#define UNIFORM_TRANSFORM layout(binding = 0) uniform TransformUniforms \
{ \
    mat4 model_to_world; \
    mat4 world_to_view; \
    mat4 view_to_clip; \
    uint object_id; \
} transform;

struct LightDescription
{
    vec3 colour;
    float multiplier;
    vec3 direction;
    float is_directional;
    vec3 position;
    float cos_half_ang_radians;
};

#define UNIFORM_SCENE layout(binding = 1) uniform SceneUniforms \
{ \
	vec2 viewport_size; \
    float time; \
    LightDescription[16] lights; \
} scene;

#define UNIFORM_OFFSET 2

#define VARYING_COMMON(io) layout(location = 0) io CommonVaryings \
{ \
    vec3 position; \
    vec3 colour; \
    vec3 normal; \
    vec3 tangent; \
    vec3 bitangent; \
    vec2 uv; \
    vec3 world_position; \
    vec3 world_normal; \
} varyings;

#define VARYING_OFFSET 8

#define STANDARD_VARYING_MATH varyings.position = vert_position; \
varyings.colour = vert_colour; \
varyings.normal = vert_normal; \
varyings.tangent = vert_tangent; \
varyings.bitangent = cross(varyings.normal, varyings.tangent); \
varyings.uv = vert_uv; \
varyings.world_position = (transform.model_to_world * vec4(varyings.position, 1.0)).xyz; \
varyings.world_normal = (normalize(transform.model_to_world * vec4(varyings.normal, 0.0))).xyz; \
gl_Position = transform.view_to_clip * transform.world_to_view * vec4(varyings.world_position, 1.0f);

#define FRAGMENT_OUTPUTS layout(location = 0) out vec4 frag_colour; \
layout(location = 1) out vec4 frag_normal; \
layout(location = 2) out vec4 frag_custom;