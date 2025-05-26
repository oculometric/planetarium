#pragma once

#include "vector4.h"
#include "vector3.h"
#include "matrix4.h"
#include "quaternion.h"
#include "vector"

class PTNode;

class PTTransform
{
private:
    PTVector3f local_position{ 0, 0, 0 };
    PTQuaternion local_rotation{ 1, 0, 0, 0 };
    PTVector3f local_scale{ 1, 1, 1 };

    PTMatrix4f local_to_parent;
    PTMatrix4f local_to_world;

    PTNode* node = nullptr;

    PTTransform* parent = nullptr;
    std::vector<PTTransform*> children;

public:
    inline PTTransform(PTNode* _node, PTVector3f position = { 0, 0, 0 }, PTQuaternion rotation = { 0, 0, 0, 1 }, PTVector3f scale = { 1, 1, 1 }) : local_position(position), local_rotation(rotation), local_scale(scale)
    {
        node = _node;
        updateLocalFromParams();
        updateWorldFromLocal();
    }

    inline PTVector3f getLocalPosition() const { return local_position; }
    inline PTQuaternion getLocalRotation() const { return local_rotation; }
    inline PTVector3f getLocalScale() const { return local_scale; }

    PTVector3f getPosition() const;

    PTVector3f getRight() const;
    PTVector3f getUp() const;
    PTVector3f getForward() const;

    // TODO: getters, world space

    inline void setLocalPosition(PTVector3f position) { local_position = position; updateLocalFromParams(); updateWorldFromLocal(); }
    inline void setLocalRotation(PTQuaternion rotation) { local_rotation = rotation; updateLocalFromParams(); updateWorldFromLocal(); }
    inline void setLocalScale(PTVector3f scale) { local_scale = scale; updateLocalFromParams(); updateWorldFromLocal(); }
    // TODO: setters, world space

    inline PTMatrix4f getLocalToParent() { return local_to_parent; }
    inline PTMatrix4f getLocalToWorld() { return local_to_world; }

    void translate(PTVector3f vector);
    void rotate(float degrees, PTVector3f axis, PTVector3f around = { 0, 0, 0 });
    
    void setParent(PTTransform* new_parent, bool preserve_world_transform = false);
	inline PTTransform* getParent() { return parent; }
	inline std::vector<PTTransform*> getChildren() { return children; }
    
private:
    static PTMatrix4f buildTranslationMatrix(PTVector3f translation);
    static PTMatrix4f buildRotationMatrix(PTQuaternion rotation);
    static PTMatrix4f buildScalingMatrix(PTVector3f scaling);

    // TODO: update functions
    void updateLocalFromParams();
    void updateParamsFromLocal();
    void updateWorldFromLocal();
    void updateLocalFromWorld();
    void updateChildren();
};