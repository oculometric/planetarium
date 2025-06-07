#pragma once

#include "node.h"
#include "reference_counter.h"

typedef PTCountedPointer<class PTMesh_T> PTMesh;
typedef PTCountedPointer<class PTMaterial_T> PTMaterial;

class PTMeshNode : public PTNode
{
    friend class PTResourceManager;
private:
    PTMesh mesh_data = nullptr;
    PTMaterial material = nullptr;

protected:
    PTMeshNode(PTDeserialiser::ArgMap arguments);
    ~PTMeshNode();

public:
    inline PTMesh getMesh() const;
    inline PTMaterial getMaterial() const;
    void setMesh(PTMesh _mesh_data);
    void setMaterial(PTMaterial _material);
};