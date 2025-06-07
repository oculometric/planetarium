#pragma once

#include "node.h"
#include "reference_counter.h"

typedef PTCountedPointer<class PTMesh_T> PTMesh;
typedef PTCountedPointer<class PTMaterial_T> PTMaterial;

class PTGizmoNode_T : public PTNode_T
{
    friend class PTNode_T;
private:
    PTNode tracking_node = nullptr;
    PTMesh axes_mesh = nullptr;
    PTMaterial material = nullptr;

public:
	~PTGizmoNode_T();

    virtual void process(float delta_time) override;

protected:
    PTGizmoNode_T(PTDeserialiser::ArgMap arguments);
};

typedef PTCountedPointer<PTGizmoNode_T> PTGizmoNode;
