#include "transform.h"

void PTTransform::updateLocalFromParams()
{
    PTMatrix4f translation
    {
        1.0f, 0.0f, 0.0f, local_position.x,
        0.0f, 1.0f, 0.0f, local_position.y,
        0.0f, 0.0f, 1.0f, local_position.z,
        0.0f, 0.0f, 0.0f, 1.0f
    };

    // TODO: replace this with quat-to-matrix
    PTVector4f rot = local_rotation * ((float)M_PI / 180.0f);

    PTMatrix4f rotation_x
    {
        1, 0,           0,            0,
        0, cosf(rot.x), -sinf(rot.x), 0,
        0, sinf(rot.x), cosf(rot.x),  0,
        0, 0,           0,            1
    };

    PTMatrix4f rotation_y
    {
        cosf(rot.y),  0, sinf(rot.y), 0,
        0,            1, 0,           0,
        -sinf(rot.y), 0, cosf(rot.y), 0,
        0,            0, 0,           1
    };

    PTMatrix4f rotation_z
    {
        cosf(rot.z), -sinf(rot.z), 0, 0,
        sinf(rot.z), cosf(rot.z),  0, 0,
        0,           0,            1, 0,
        0,           0,            0, 1
    };


    PTMatrix4f stretch
    {
        local_scale.x, 0.0f,          0.0f,          0.0f,
        0.0f,          local_scale.y, 0.0f,          0.0f,
        0.0f,          0.0f,          local_scale.z, 0.0f,
        0.0f,          0.0f,          0.0f,          1.0f
    };

    local_to_parent = translation * rotation_z * rotation_x * rotation_y * stretch;
}

void PTTransform::updateWorldFromLocal()
{
    if (parent == nullptr)
        local_to_world = local_to_parent;
    else
        local_to_world = parent->local_to_world * local_to_parent;
}
