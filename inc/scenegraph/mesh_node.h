#pragma once

#include "node.h"

class PTMesh;

// TODO: mesh node class
class PTMeshNode : public PTNode
{
    friend class PTResourceManager;
private:
    PTMesh* mesh_data = nullptr;
public:

protected:
    PTMeshNode(PTDeserialiser::ArgMap arguments);
    ~PTMeshNode();
};