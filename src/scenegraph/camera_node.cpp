#include "camera_node.h"

PTMatrix4f PTCameraNode::projectionMatrix(float near_clip_plane, float far_clip_plane, float horizontal_fov_degrees, float aspect_ratio)
{
    // actual hell magic. hail satan
    float clip_rat = -far_clip_plane / (far_clip_plane - near_clip_plane);
    float s_x = 1.0f / tanf((horizontal_fov_degrees / 2.0f) * ((float)M_PI / 180.0f));
    float s_y = s_x * aspect_ratio;

    PTMatrix4f projection
    {
        s_x,     0.0f,       0.0f,       0.0f,
        0.0f,    -s_y,       0.0f,       0.0f,
        0.0f,    0.0f,       clip_rat,   clip_rat * near_clip_plane,
        0.0f,    0.0f,       -1.0f,      0.0f
    };

    return projection;
}

PTCameraNode::PTCameraNode(PTDeserialiser::ArgMap arguments) : PTNode(arguments)
{
    // transfer the camera config from args
    getArg(arguments, "near_clip", near_clip);
    getArg(arguments, "far_clip", far_clip);
    getArg(arguments, "fov", horizontal_fov);
}
