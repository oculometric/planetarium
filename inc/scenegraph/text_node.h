#pragma once

#include "node.h"

class PTMesh;

class PTTextNode : public PTNode
{
	friend class PTResourceManager;
public:
	std::string text = "Hello, World!";

private:
	PTMesh* mesh = nullptr;
	PTMaterial* material = nullptr;

protected:
	PTTextNode(PTDeserialiser::ArgMap arguments);
	~PTTextNode();
};