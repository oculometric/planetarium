#pragma once

#include "node.h"

class PTMesh;

class PTTextNode : public PTNode
{
	friend class PTResourceManager;
private:
	std::string text = "Hello, World!";
	PTMesh* mesh = nullptr;
	PTMaterial* material = nullptr;

public:
	void updateText(std::string _text);

protected:
	PTTextNode(PTDeserialiser::ArgMap arguments);
	~PTTextNode();
};