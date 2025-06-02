#pragma once

#include "node.h"
#include "scenegraph/fly_camera_node.h"
#include "scenegraph/light_node.h"
#include "scenegraph/text_node.h"
#include "scenegraph/gizmo_node.h"
#include "scenegraph/camera_node.h"
#include "scenegraph/mesh_node.h"

static map<string, PTNodeInstantiateFunc> node_instantiators = 
{
	INSTANTIATE_FUNC(Node),
	INSTANTIATE_FUNC(FlyCameraNode),
	INSTANTIATE_FUNC(LightNode),
	INSTANTIATE_FUNC(TextNode),
	INSTANTIATE_FUNC(GizmoNode),
	INSTANTIATE_FUNC(CameraNode),
	INSTANTIATE_FUNC(MeshNode)
};
