#pragma once

#include "transform.h"
#include "resource.h"
#include "deserialiser.h"

class PTApplication;
class PTScene;

class PTNode : public PTResource
{
	friend class PTResourceManager;
public:
	std::string name = "Node";

private:
	PTTransform transform = PTTransform(this);

public:
	inline PTTransform* getTransform() { return &transform; }
	PTScene* getScene();
	PTApplication* getApplication();

	PTNode(PTNode& other) = delete;
    PTNode(PTNode&& other) = delete;
    PTNode operator=(PTNode& other) = delete;
    PTNode operator=(PTNode&& other) = delete;

	inline virtual void process(float delta_time) { }

protected:
	// called to initialise the node with default values
	inline PTNode() { }
	
	// called to deserialise a map of named arguments and populate the node config with arguments
	// when the node references a resource, it should be added as dependency (incrementing that resource's reference count)
	PTNode(PTDeserialiser::ArgMap arguments);

	// called to destroy the node. referenced resources should be removed as dependencies (decrementing the resource's reference count)
	inline ~PTNode() { }
};