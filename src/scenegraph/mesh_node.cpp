#include "mesh_node.h"

#include "material.h"
#include "debug.h"
#include "mesh.h"
#include "render_server.h"

PTMeshNode::PTMeshNode(PTDeserialiser::ArgMap arguments) : PTNode(arguments)
{
    // if there's a mesh data argument, cast and make it our mesh
    if (hasArg(arguments, "data", PTDeserialiser::ArgType::RESOURCE_ARG))
        setMesh(dynamic_cast<PTMesh*>(arguments["data"].r_val));
    // if there's a material, also set it
    if (hasArg(arguments, "material", PTDeserialiser::ArgType::RESOURCE_ARG))
        setMaterial(dynamic_cast<PTMaterial*>(arguments["material"].r_val));
}

void PTMeshNode::setMesh(PTMesh* _mesh_data)
{
    // delete draw request
    PTRenderServer::get()->removeAllDrawRequests(this);
    
    // unlink mesh data
    if (mesh_data != nullptr)
        removeDependency(mesh_data);

    // link new mesh data if not nullptr, and add draw request
    mesh_data = _mesh_data;
    if (mesh_data != nullptr)
    {
        addDependency(mesh_data);
        PTRenderServer::get()->addDrawRequest(this, mesh_data, material);
    }
}

void PTMeshNode::setMaterial(PTMaterial* _material)
{
    // delete draw request
    PTRenderServer::get()->removeAllDrawRequests(this);
    
    // unlink material
    if (material != nullptr)
        removeDependency(material);

    // link new mesh data if not nullptr, and add draw request
    material = _material;
    if (material != nullptr)
    {
        addDependency(material);
        PTRenderServer::get()->addDrawRequest(this, mesh_data, material);
    }
}

PTMeshNode::~PTMeshNode()
{
    // if there was mesh data set, remove it
    if (mesh_data != nullptr)
        removeDependency(mesh_data);
    mesh_data = nullptr;
    if (material != nullptr)
        removeDependency(material);
    material = nullptr;
    
    // remove the draw requests associated with this node
    PTRenderServer::get()->removeAllDrawRequests(this);
}