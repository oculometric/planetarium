#pragma once

#include "node.h"
#include "reference_counter.h"

typedef PTCountedPointer<class PTMesh_T> PTMesh;
typedef PTCountedPointer<class PTMaterial_T> PTMaterial;

class PTMeshNode_T : public PTNode_T
{
    friend class PTNode_T;
private:
    PTMesh mesh_data = nullptr;
    PTMaterial material = nullptr;

public:
    ~PTMeshNode_T();

    PTMesh getMesh() const;
    PTMaterial getMaterial() const;
    void setMesh(PTMesh _mesh_data);
    void setMaterial(PTMaterial _material);

protected:
    PTMeshNode_T(PTDeserialiser::ArgMap arguments);
};

typedef PTCountedPointer<PTMeshNode_T> PTMeshNode;
