#include "mesh_node.h"

#include "material.h"
#include "debug.h"
#include "mesh.h"
#include "render_server.h"

PTMeshNode_T::PTMeshNode_T(PTDeserialiser::ArgMap arguments) : PTNode_T(arguments)
{
    // if there's a mesh data argument, cast and make it our mesh
    if (hasArg(arguments, "data", PTDeserialiser::ArgType::RESOURCE_ARG))
        setMesh(arguments["data"].r_val);
    // if there's a material, also set it
    if (hasArg(arguments, "material", PTDeserialiser::ArgType::RESOURCE_ARG))
        setMaterial(arguments["material"].r_val);
}

PTMesh PTMeshNode_T::getMesh() const { return mesh_data; }

PTMaterial PTMeshNode_T::getMaterial() const { return material; }

void PTMeshNode_T::setMesh(PTMesh _mesh_data)
{
    // delete draw request
    PTRenderServer::get()->removeAllDrawRequests(this);
    
    // link new mesh data if not nullptr, and add draw request
    mesh_data = _mesh_data;
    PTRenderServer::get()->addDrawRequest(PTNode(this), mesh_data, material);
}

void PTMeshNode_T::setMaterial(PTMaterial _material)
{
    // delete draw request
    PTRenderServer::get()->removeAllDrawRequests(this);
    
    // link new mesh data if not nullptr, and add draw request
    material = _material;
    PTRenderServer::get()->addDrawRequest(PTNode(this), mesh_data, material);
}

PTMeshNode_T::~PTMeshNode_T()
{
    // if there was mesh data set, remove it
    mesh_data = PTMesh(nullptr);
    material = PTMaterial(nullptr);
    
    // remove the draw requests associated with this node
    PTRenderServer::get()->removeAllDrawRequests(this);
}