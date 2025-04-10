#include "mesh_node.h"

#include "debug.h"
#include "mesh.h"
#include "render_server.h"

PTMeshNode::PTMeshNode(PTDeserialiser::ArgMap arguments) : PTNode(arguments)
{
    // if there's a mesh data argument, cast and make it our mesh
    if (hasArg(arguments, "data", PTDeserialiser::ArgType::RESOURCE_ARG))
        setMesh((PTMesh*)arguments["data"].r_val);
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
        PTRenderServer::get()->addDrawRequest(this, mesh_data);
    }
}

PTMeshNode::~PTMeshNode()
{
    // if there was mesh data set, remove it
    if (mesh_data != nullptr)
        removeDependency(mesh_data);
    mesh_data = nullptr;
    
    // remove the draw requests associated with this node
    PTRenderServer::get()->removeAllDrawRequests(this);
}