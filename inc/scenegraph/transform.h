#pragma once

#include "vector4.h"
#include "vector3.h"
#include "matrix3.h"
#include "quaternion.h"

class PTTransform
{
private:
    PTVector3f local_position{ 0, 0, 0 };
    PTQuaternion local_rotation{ 1, 0, 0, 0 }; // TODO: quaternion class/library
    PTVector3f local_scale{ 1, 1, 1 };

    PTMatrix3f local_to_parent;
    PTMatrix3f local_to_world;

    PTTransform* parent = nullptr;

public:
    inline PTTransform(PTVector3f position = { 0, 0, 0 }, PTQuaternion rotation = { 1, 0, 0, 0 }, PTVector3f scale = { 1, 1, 1 }) : local_position(position), local_rotation(rotation), local_scale(scale)
    {
        updateLocalFromParams();
        updateWorldFromLocal();
    }

    inline PTVector3f getLocalPosition() { return local_position; }
    inline PTQuaternion getLocalRotation() { return local_rotation; }
    inline PTVector3f getLocalScale() { return local_scale; }
    // TODO: getters, world space

    inline void setLocalPosition(PTVector3f position) { local_position = position; updateLocalFromParams(); updateWorldFromLocal(); }
    inline void setLocalRotation(PTQuaternion rotation) { local_rotation = rotation; updateLocalFromParams(); updateWorldFromLocal(); }
    inline void setLocalScale(PTVector3f scale) { local_scale = scale; updateLocalFromParams(); updateWorldFromLocal(); }
    // TODO: setters, world space

    inline PTMatrix3f getLocalToParent() { return local_to_parent; }
    inline PTMatrix3f getLocalToWorld() { return local_to_world; }

    // TODO: transform functions
    // TODO: child/parent system
    
private:
    // TODO: update functions
    void updateLocalFromParams();
    void updateParamsFromLocal();
    void updateWorldFromLocal();
    void updateLocalFromWorld();

};