#include "gizmo_node.h"

#include "resource_manager.h"
#include "application.h"
#include "scene.h"

void PTGizmoNode::process(float delta_time)
{
    PTVector3f forward = -getScene()->getCamera()->getTransform()->getForward();
    PTVector3f origin = getScene()->getCamera()->getTransform()->getPosition();
    //auto nodes = getScene()->getNodes();

    // TODO: find node currently under camera, set draw request to be the axes placed at that object origin
    // TODO: use an entirely vertex-colour-based unlit shader
    getTransform()->setLocalPosition(origin + (forward * 3.0f));
}

PTGizmoNode::PTGizmoNode(PTDeserialiser::ArgMap arguments)
{
    axes_mesh = PTResourceManager::get()->createMesh("res/debug_axes.obj");
    addDependency(axes_mesh, false);
    getApplication()->addDrawRequest(PTDrawRequest{ axes_mesh }, this);
}

PTGizmoNode::~PTGizmoNode()
{
    getApplication()->removeAllDrawRequests(this);
    removeDependency(axes_mesh);
}
