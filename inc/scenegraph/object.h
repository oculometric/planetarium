#pragma once

#include "vector3.h"
#include "matrix4.h"

class PTObject
{
public:
	PTVector3f local_position = PTVector3f{ 0, 0, 0 };
	PTVector3f local_rotation = PTVector3f{ 0, 0, 0 };
	PTVector3f local_scale = PTVector3f{ 1, 1, 1 };

private:

public:
	inline PTMatrix4f getLocalRotation()
	{
		PTVector3f rot = local_rotation * (M_PI / 180.0);

		PTMatrix4f rotation_x
		{
			1, 0,           0,            0,
			0, cosf(rot.x), -sinf(rot.x), 0,
			0, sinf(rot.x), cosf(rot.x),  0,
			0, 0,           0,            1
		};

		PTMatrix4f rotation_y
		{
			cosf(rot.y),  0, sinf(rot.y), 0,
			0,            1, 0,           0,
			-sinf(rot.y), 0, cosf(rot.y), 0,
			0,            0, 0,           1
		};

		PTMatrix4f rotation_z
		{
			cosf(rot.z), -sinf(rot.z), 0, 0,
			sinf(rot.z), cosf(rot.z),  0, 0,
			0,           0,            1, 0,
			0,           0,            0, 1
		};

		return rotation_z * rotation_x * rotation_y;
	}

	inline PTMatrix4f getLocalTransform()
	{
		PTMatrix4f translation
		{
			1.0f, 0.0f, 0.0f, local_position.x,
			0.0f, 1.0f, 0.0f, local_position.y,
			0.0f, 0.0f, 1.0f, local_position.z,
			0.0f, 0.0f, 0.0f, 1.0f
		};
		
		PTMatrix4f stretch
		{
			local_scale.x, 0.0f,          0.0f,          0.0f,
			0.0f,          local_scale.y, 0.0f,          0.0f,
			0.0f,          0.0f,          local_scale.z, 0.0f,
			0.0f,          0.0f,          0.0f,          1.0f
		};

		return translation * getLocalRotation() * stretch;
	}
};