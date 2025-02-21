#include "mesh_node.h"

#include "debug.h"
#include "mesh.h"
#include "application.h"

PTMeshNode::PTMeshNode(PTDeserialiser::ArgMap arguments) : PTNode(arguments)
{
    if (hasArg(arguments, "data", PTDeserialiser::ArgType::RESOURCE_ARG))
    {
        mesh_data = (PTMesh*)arguments["data"].r_val;
        getApplication()->addDrawRequest(DrawRequest{ mesh_data, getTransform() }, this);
        addDependency(mesh_data);
    }
}

PTMeshNode::~PTMeshNode()
{
    if (mesh_data != nullptr)
        removeDependency(mesh_data);
    getApplication()->removeAllDrawRequests(this);
    mesh_data = nullptr;    
}