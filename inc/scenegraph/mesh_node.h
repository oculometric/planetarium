#pragma once

#include "node.h"

class PTMesh;

class PTMeshNode : public PTNode
{
    friend class PTResourceManager;
private:
    PTMesh* mesh_data = nullptr;
    // TODO: materials, including set function

protected:
    PTMeshNode(PTDeserialiser::ArgMap arguments);

    void setMesh(PTMesh* _mesh_data);

    ~PTMeshNode();
};