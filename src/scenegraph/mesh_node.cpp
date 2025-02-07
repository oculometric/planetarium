#include "mesh_node.h"

#include "debug.h"
#include "mesh.h"

PTMeshNode::PTMeshNode(PTDeserialiser::ArgMap arguments) : PTNode(arguments)
{
    if (arguments.contains("data") && arguments["data"].type == PTDeserialiser::ArgType::RESOURCE_ARG)
    {
        mesh_data = (PTMesh*)arguments["data"].r_val;
        addDependency(mesh_data);
    }
}

PTMeshNode::~PTMeshNode()
{
    if (mesh_data != nullptr)
        removeDependency(mesh_data);
    mesh_data = nullptr;    
}