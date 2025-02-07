#pragma once

#define _USE_MATH_DEFINES
#include <math.h>

#include "node.h"
#include "matrix4.h"

class PTCameraNode : public PTNode
{
	friend class PTResourceManager;
public:
	float near_clip = 0.1f;
	float far_clip = 100.0f;
	float horizontal_fov = 120.0f;

public:
	inline PTMatrix4f getProjectionMatrix(float aspect_ratio)
	{
    	return projectionMatrix(near_clip, far_clip, horizontal_fov, aspect_ratio);
	}

	static PTMatrix4f projectionMatrix(float near_clip_plane, float far_clip_plane, float horizontal_fov_degrees, float aspect_ratio);

protected:
	PTCameraNode(PTDeserialiser::ArgMap arguments);
};