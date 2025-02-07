#pragma once

#include "transform.h"
#include "resource.h"
#include "deserialiser.h"

class PTNode : public PTResource
{
	friend class PTResourceManager;
public:
	std::string name = "Node";

private:
	PTTransform* transform = nullptr;

public:
	inline PTTransform* getTransform() { return transform; }
	PTScene* getScene();

	// TODO: parent-child system, where parent nodes depend on child nodes

	PTNode(PTNode& other) = delete;
    PTNode(PTNode&& other) = delete;
    PTNode operator=(PTNode& other) = delete;
    PTNode operator=(PTNode&& other) = delete;

protected:
	// called to initialise the node with default values
	inline virtual void create() { };
	// called to deserialise a map of named arguments and populate the node config with arguments
	// when the node references a resource, it should be added as dependency (incrementing that resource's reference count)
	inline virtual void deserialise(std::map<std::string, PTDeserialiser::Argument> arguments) { };
	// called to destroy the node. referenced resources should be removed as dependencies (decrementing the resource's reference count)
	inline virtual void cleanUp() { };

	// TODO: how do i make these private?
	inline PTNode() { transform = new PTTransform(); create(); }
	// TODO: will this call the child class implementation of create and deserialise???
	inline PTNode(std::map<std::string, PTDeserialiser::Argument> arguments) { transform = new PTTransform(); create(); deserialise(arguments); }

	// TODO: will this call the child class implementation of cleanUp????
	inline ~PTNode() { delete transform; cleanUp(); }
};
// public:
// 	PTVector3f local_position = PTVector3f{ 0, 0, 0 };
// 	PTVector3f local_rotation = PTVector3f{ 0, 0, 0 };
// 	PTVector3f local_scale = PTVector3f{ 1, 1, 1 };

// private:

// public:
// 	inline PTMatrix4f getLocalRotation()
// 	{
// 		PTVector3f rot = local_rotation * (M_PI / 180.0);

// 		PTMatrix4f rotation_x
// 		{
// 			1, 0,           0,            0,
// 			0, cosf(rot.x), -sinf(rot.x), 0,
// 			0, sinf(rot.x), cosf(rot.x),  0,
// 			0, 0,           0,            1
// 		};

// 		PTMatrix4f rotation_y
// 		{
// 			cosf(rot.y),  0, sinf(rot.y), 0,
// 			0,            1, 0,           0,
// 			-sinf(rot.y), 0, cosf(rot.y), 0,
// 			0,            0, 0,           1
// 		};

// 		PTMatrix4f rotation_z
// 		{
// 			cosf(rot.z), -sinf(rot.z), 0, 0,
// 			sinf(rot.z), cosf(rot.z),  0, 0,
// 			0,           0,            1, 0,
// 			0,           0,            0, 1
// 		};

// 		return rotation_z * rotation_x * rotation_y;
// 	}

// 	inline PTMatrix4f getLocalTransform()
// 	{
// 		PTMatrix4f translation
// 		{
// 			1.0f, 0.0f, 0.0f, local_position.x,
// 			0.0f, 1.0f, 0.0f, local_position.y,
// 			0.0f, 0.0f, 1.0f, local_position.z,
// 			0.0f, 0.0f, 0.0f, 1.0f
// 		};
		
// 		PTMatrix4f stretch
// 		{
// 			local_scale.x, 0.0f,          0.0f,          0.0f,
// 			0.0f,          local_scale.y, 0.0f,          0.0f,
// 			0.0f,          0.0f,          local_scale.z, 0.0f,
// 			0.0f,          0.0f,          0.0f,          1.0f
// 		};

// 		return translation * getLocalRotation() * stretch;
// 	}