#pragma once

#include "node.h"

class PTMesh;

class PTMeshNode : public PTNode
{
    friend class PTResourceManager;
private:
    PTMesh* mesh_data = nullptr;
    // TODO: materials

protected:
    PTMeshNode(PTDeserialiser::ArgMap arguments);
    ~PTMeshNode();
};