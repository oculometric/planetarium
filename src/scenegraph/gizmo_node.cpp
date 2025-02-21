#include "gizmo_node.h"

#include "resource_manager.h"
#include "application.h"
#include "scene.h"

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
        debugLog("new debug target is " + to_string((uint64_t)(min_node)));
        getApplication()->removeAllDrawRequests(this);
        if (min_node != nullptr)
        {
            getApplication()->addDrawRequest(DrawRequest{ axes_mesh, min_node->getTransform() }, this);
            debugSetObjectProperty("name", min_node->name);
            debugSetObjectProperty("position", to_string(min_node->getTransform()->getPosition()));
            debugSetObjectProperty("rotation", to_string(min_node->getTransform()->getLocalRotation()));
            debugSetObjectProperty("scale", to_string(min_node->getTransform()->getLocalScale()));
        }
        else
        {
            debugClearObjectProperty("name");
            debugClearObjectProperty("position");
            debugClearObjectProperty("rotation");
            debugClearObjectProperty("scale");
        }
        
        tracking_node = min_node;
    }

    // TODO: use an entirely vertex-colour-based unlit shader, which draws on top of other things (ignores depth)
}

PTGizmoNode::PTGizmoNode(PTDeserialiser::ArgMap arguments)
{
    axes_mesh = PTResourceManager::get()->createMesh("res/debug_axes.obj");
    addDependency(axes_mesh, false);
}

PTGizmoNode::~PTGizmoNode()
{
    getApplication()->removeAllDrawRequests(this);
    removeDependency(axes_mesh);
}
