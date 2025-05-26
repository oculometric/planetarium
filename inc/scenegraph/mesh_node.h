#pragma once

#include "node.h"

class PTMaterial;
class PTMesh;

class PTMeshNode : public PTNode
{
    friend class PTResourceManager;
private:
    PTMesh* mesh_data = nullptr;
    PTMaterial* material = nullptr;

protected:
    PTMeshNode(PTDeserialiser::ArgMap arguments);
    ~PTMeshNode();

public:
    inline PTMesh* getMesh() const { return mesh_data; }
    inline PTMaterial* getMaterial() const { return material; }
    void setMesh(PTMesh* _mesh_data);
    void setMaterial(PTMaterial* _material);
};