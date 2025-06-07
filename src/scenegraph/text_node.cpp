#include "text_node.h"

#include "mesh.h"
#include "material.h"
#include "resource_manager.h"
#include "render_server.h"

#define min(a,b) a > b ? b : a

void PTTextNode_T::setText(std::string _text)
{
    text = _text;

    memcpy((void*)(&(uniform_buffer.text)), text.data(), min(text.size(), 64 * 4 * 4));

    updateUniforms();
}

void PTTextNode_T::updateUniforms()
{
    uniform_buffer.aspect_ratio = getTransform()->getLocalScale().y / getTransform()->getLocalScale().x;
    material->setUniform(2, uniform_buffer);
}

PTTextNode_T::PTTextNode_T(PTDeserialiser::ArgMap arguments) : PTNode_T(arguments)
{
    material = PTMaterial_T::createMaterial("res/engine/material/text.ptmat");
    mesh = PTMesh_T::createMesh("res/engine/mesh/plane.obj");

    PTRenderServer::get()->addDrawRequest(this, mesh, material);

    getArg(arguments, "text", text);
    getArg(arguments, "line_width", uniform_buffer.characters_per_line);
    getArg(arguments, "text_colour", uniform_buffer.text_colour);
    getArg(arguments, "background_colour", uniform_buffer.background_colour);

    setText(text);
}

PTTextNode_T::~PTTextNode_T()
{
    PTRenderServer::get()->removeAllDrawRequests(this);

    mesh = nullptr;
    material = nullptr;
}

