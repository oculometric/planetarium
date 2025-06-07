#pragma once

#include "node.h"
#include "reference_counter.h"

typedef PTCountedPointer<class PTMesh_T> PTMesh;
typedef PTCountedPointer<class PTMaterial_T> PTMaterial;

class PTGizmoNode : public PTNode
{
	friend class PTResourceManager;
private:
    PTNode* tracking_node = nullptr;
    PTMesh axes_mesh = nullptr;
    PTMaterial material = nullptr;

public:
    virtual void process(float delta_time) override;

protected:
	PTGizmoNode(PTDeserialiser::ArgMap arguments);
	~PTGizmoNode();
};