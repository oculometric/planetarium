#include "text_node.h"

#include "mesh.h"
#include "material.h"
#include "resource_manager.h"
#include "render_server.h"

#define min(a,b) a > b ? b : a

void PTTextNode::setText(std::string _text)
{
    text = _text;

    memcpy((void*)(&(uniform_buffer.text)), text.data(), min(text.size(), 64 * 4 * 4));

    updateUniforms();
}

void PTTextNode::updateUniforms()
{
    uniform_buffer.aspect_ratio = getTransform()->getLocalScale().y / getTransform()->getLocalScale().x;
    material->setUniform(2, uniform_buffer);
}

PTTextNode::PTTextNode(PTDeserialiser::ArgMap arguments) : PTNode(arguments)
{
    material = PTResourceManager::get()->createMaterial("res/engine/material/text.ptmat", nullptr, nullptr, true);
    mesh = PTResourceManager::get()->createMesh("res/engine/mesh/plane.obj");

    addDependency(material, false);
    addDependency(mesh, false);

    PTRenderServer::get()->addDrawRequest(this, mesh, material);

    getArg(arguments, "text", text);
    getArg(arguments, "line_width", uniform_buffer.characters_per_line);

    setText(text);
}

PTTextNode::~PTTextNode()
{
    PTRenderServer::get()->removeAllDrawRequests(this);

    removeDependency(material);
    removeDependency(mesh);
}

