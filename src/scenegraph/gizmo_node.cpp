#include "gizmo_node.h"

#include "mesh.h"
#include "resource_manager.h"
#include "render_server.h"
#include "scene.h"
#include "material.h"

using namespace std;

void PTGizmoNode::process(float delta_time)
{
    PTVector3f forward = -getScene()->getCamera()->getTransform()->getForward();
    PTVector3f origin = getScene()->getCamera()->getTransform()->getPosition();
    
    auto nodes = getScene()->getNodes();
    float min_dist = 50.0f;
    PTNode* min_node = nullptr;
    for (PTNode* node : nodes)
    {
        if (node == this) continue;
        PTVector3f camera_to_node = node->getTransform()->getPosition() - origin;
        if (mag(camera_to_node) < 0.5f)
            continue;
        float dist_along_line = camera_to_node ^ forward;
        float dist_sqr = sq_mag(camera_to_node) - (dist_along_line * dist_along_line);
        if (dist_sqr < min_dist)
        {
            min_dist = dist_sqr;
            min_node = node;
        }
    }

    if (min_node != tracking_node)
    {
        PTRenderServer::get()->removeAllDrawRequests(this);
        if (min_node != nullptr)
        {
            debugLog("new debug target is " + min_node->name);
            PTRenderServer::get()->addDrawRequest(this, axes_mesh, material, min_node->getTransform());
            debugSetObjectProperty("name", min_node->name);
            debugSetObjectProperty("position", to_string(min_node->getTransform()->getPosition()));
            debugSetObjectProperty("rotation", to_string(min_node->getTransform()->getLocalRotation()));
            debugSetObjectProperty("scale", to_string(min_node->getTransform()->getLocalScale()));
        }
        else
        {
            debugLog("debug target lost");
            debugClearObjectProperty("name");
            debugClearObjectProperty("position");
            debugClearObjectProperty("rotation");
            debugClearObjectProperty("scale");
        }
        
        tracking_node = min_node;
    }
}

PTGizmoNode::PTGizmoNode(PTDeserialiser::ArgMap arguments)
{
    axes_mesh = PTMesh_T::createMesh("res/engine/mesh/debug_axes.obj");
    material = PTMaterial_T::createMaterial("res/engine/material/unlit_overlay.ptmat");
}

PTGizmoNode::~PTGizmoNode()
{
    PTRenderServer::get()->removeAllDrawRequests(this);
    axes_mesh = nullptr;
    material = nullptr;
}
