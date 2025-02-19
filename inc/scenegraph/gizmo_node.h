#pragma once

#include "node.h"

class PTMesh;

class PTGizmoNode : public PTNode
{
	friend class PTResourceManager;
private:
    PTNode* tracking_node = nullptr;
    PTMesh* axes_mesh = nullptr;

public:
    virtual void process(float delta_time) override;

protected:
	PTGizmoNode(PTDeserialiser::ArgMap arguments);
	~PTGizmoNode();
};