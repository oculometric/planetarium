#pragma once

#include "transform.h"
#include "resource.h"
#include "deserialiser.h"
#include "reference_counter.h"

class PTApplication;

class PTNode_T
{
public:
	std::string name = "Node";

private:
	PTTransform transform = PTTransform(this);
	PTScene_T* scene = nullptr; // FIXME: make this a counted pointer issued by scene (scene keeps it so it always has one)

public:
	PTNode_T(PTNode& other) = delete;
	PTNode_T(PTNode&& other) = delete;
	PTNode_T operator=(PTNode& other) = delete;
	PTNode_T operator=(PTNode&& other) = delete;
	// called to destroy the node. referenced resources should be removed as dependencies (decrementing the resource's reference count)
	~PTNode_T();

	template<class T>
	static inline PTCountedPointer<T> createNode(std::string name, PTScene_T* scene, PTDeserialiser::ArgMap arguments)
	{
		static_assert(std::is_base_of<PTNode_T, T>::value || std::is_same<PTNode_T, T>::value, "T is not a PTNode type");
		T* tmp = new T(arguments);
		tmp->name = name;
		tmp->scene = scene;
		return PTCountedPointer<T>(tmp);
	}

	inline PTTransform* getTransform() { return &transform; }

	// called to deserialise a map of named arguments and populate the node config with arguments
	inline virtual void process(float delta_time) { }

protected:
	// called to initialise the node with default values
	PTNode_T(PTDeserialiser::ArgMap arguments);

	PTScene_T* getScene() const;
	PTApplication* getApplication();
};

typedef PTCountedPointer<PTNode_T> PTNode;

/**
 * template for custom node types
 * 
 * class PTCustomNode_T : public PTNode_T
{
private:
	// private variables

public:
	~PTCustomNode_T();

	// public functions
	virtual void configure(PTDeserialiser::ArgMap arguments) override;
};

typedef PTCountedPointer<PTCustomNode_T> PTCustomNode;
 */