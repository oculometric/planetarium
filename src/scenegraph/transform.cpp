#include "transform.h"

PTVector3f PTTransform::getPosition() const
{
    PTVector4f vec = local_to_world.col3();
    return PTVector3f{ vec.x, vec.y, vec.z };
}

PTVector3f PTTransform::getRight() const
{
    PTVector4f vec = local_to_world.col0();
    return PTVector3f{ vec.x, vec.y, vec.z };
}

PTVector3f PTTransform::getUp() const
{
    PTVector4f vec = local_to_world.col1();
    return PTVector3f{ vec.x, vec.y, vec.z };
}

PTVector3f PTTransform::getForward() const
{
    PTVector4f vec = local_to_world.col2();
    return PTVector3f{ vec.x, vec.y, vec.z };
}

void PTTransform::translate(PTVector3f vector)
{
    local_to_world = buildTranslationMatrix(vector) * local_to_world;

    updateLocalFromWorld();
    updateParamsFromLocal();
}

void PTTransform::rotate(float degrees, PTVector3f axis, PTVector3f around)
{
    local_to_world = buildTranslationMatrix(around) * buildRotationMatrix(PTQuaternion(axis, degrees)) * buildTranslationMatrix(-around) * local_to_world;

    updateLocalFromWorld();
    updateParamsFromLocal();
}

void PTTransform::setParent(PTTransform* new_parent, bool preserve_world_transform)
{
    // TODO: support preserving the world transform

    if (parent != nullptr)
    {
        auto find = parent->children.begin();
        while (*find != this)
            find++;
        parent->children.erase(find);
    }

    parent = new_parent;
    parent->children.push_back(this);

    if (preserve_world_transform)
    {
        updateLocalFromWorld();
        updateParamsFromLocal();
    }
    else
        updateWorldFromLocal();
}

PTMatrix4f PTTransform::buildTranslationMatrix(PTVector3f translation)
{
    PTMatrix4f matrix
    {
        1.0f, 0.0f, 0.0f, translation.x,
        0.0f, 1.0f, 0.0f, translation.y,
        0.0f, 0.0f, 1.0f, translation.z,
        0.0f, 0.0f, 0.0f, 1.0f
    };

    return matrix;
}

PTMatrix4f PTTransform::buildRotationMatrix(PTQuaternion rotation)
{
    // TODO: replace this with quaternion to matrix
    //PTVector4f rot = rotation * ((float)M_PI / 180.0f);

    // PTMatrix4f rotation_x
    // {
    //     1, 0,           0,            0,
    //     0, cosf(rot.x), -sinf(rot.x), 0,
    //     0, sinf(rot.x), cosf(rot.x),  0,
    //     0, 0,           0,            1
    // };

    // PTMatrix4f rotation_y
    // {
    //     cosf(rot.y),  0, sinf(rot.y), 0,
    //     0,            1, 0,           0,
    //     -sinf(rot.y), 0, cosf(rot.y), 0,
    //     0,            0, 0,           1
    // };

    // PTMatrix4f rotation_z
    // {
    //     cosf(rot.z), -sinf(rot.z), 0, 0,
    //     sinf(rot.z), cosf(rot.z),  0, 0,
    //     0,           0,            1, 0,
    //     0,           0,            0, 1
    // };

    // return rotation_z * rotation_x * rotation_y;
    return mat(rotation);
}

PTMatrix4f PTTransform::buildScalingMatrix(PTVector3f scaling)
{
    PTMatrix4f matrix
    {
        scaling.x, 0.0f,          0.0f,          0.0f,
        0.0f,          scaling.y, 0.0f,          0.0f,
        0.0f,          0.0f,          scaling.z, 0.0f,
        0.0f,          0.0f,          0.0f,      1.0f
    };

    return matrix;
}

void PTTransform::updateLocalFromParams()
{
    PTMatrix4f translation = buildTranslationMatrix(local_position);
    PTMatrix4f rotation = buildRotationMatrix(local_rotation);    
    PTMatrix4f scaling = buildScalingMatrix(local_scale);

    local_to_parent = translation * rotation * scaling;

    updateChildren();
}

void PTTransform::updateParamsFromLocal()
{
    PTMatrix4f working = local_to_parent;
    PTVector4f translation = working.col3();
    working.w_0 = working.w_1 = working.w_2 = working.w_3 = 0;
    // FIXME: does not support negative scale!
    PTVector3f scale = PTVector3f{ mag(working.col0()), mag(working.col1()), mag(working.col2()) };
    working.x_0 /= scale.x; working.x_1 /= scale.x; working.x_2 /= scale.x; working.x_3 /= scale.x;
    working.y_0 /= scale.y; working.y_1 /= scale.y; working.y_2 /= scale.y; working.y_3 /= scale.y;
    working.z_0 /= scale.z; working.z_1 /= scale.z; working.z_2 /= scale.z; working.z_3 /= scale.z;
    PTQuaternion rotation = PTQuaternion(working);
    local_position = PTVector3f{ translation.x, translation.y, translation.z };
    local_rotation = rotation;
    local_scale = scale;
}

void PTTransform::updateWorldFromLocal()
{
    if (parent == nullptr)
        local_to_world = local_to_parent;
    else
        local_to_world = parent->local_to_world * local_to_parent;    
}

void PTTransform::updateLocalFromWorld()
{
    if (parent == nullptr)
        local_to_parent = local_to_world;
    else
        local_to_parent = ~(parent->local_to_world) * local_to_world;
    
    updateChildren();
}

void PTTransform::updateChildren()
{
    for (PTTransform* child : children)
    {
        child->updateWorldFromLocal();
    }
}
