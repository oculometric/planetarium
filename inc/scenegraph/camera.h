#pragma once

#define _USE_MATH_DEFINES
#include <math.h>

#include "node.h"
#include "matrix4.h"

class PTCameraNode : public PTNode
{
public:
	float near_clip = 0.1f;
	float far_clip = 100.0f;
	float horizontal_fov = 120.0f;
	float aspect_ratio = 4.0 / 3.0;

private:

public:
	inline PTMatrix4f getProjectionMatrix()
	{
    	return projectionMatrix(near_clip, far_clip, horizontal_fov, aspect_ratio);
	}

	inline static PTMatrix4f projectionMatrix(float near_clip_plane, float far_clip_plane, float horizontal_fov_degrees, float aspect_ratio)
	{
		float clip_rat = -far_clip_plane / (far_clip_plane - near_clip_plane);
		float s_x = 1.0f / tanf((horizontal_fov_degrees / 2.0f) * (M_PI / 180.0f));
		float s_y = s_x * aspect_ratio;

		PTMatrix4f projection
		{
			s_x,     0.0f,       0.0f,       0.0f,
			0.0f,    -s_y,       0.0f,       0.0f,
			0.0f,    0.0f,       clip_rat,   clip_rat * near_clip_plane,
			0.0f,    0.0f,       -1.0f,      0.0f
		};

		return projection;
	}
};