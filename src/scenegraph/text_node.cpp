#include "text_node.h"

#include "mesh.h"
#include "material.h"
#include "resource_manager.h"
#include "render_server.h"

struct TextBuffer
{
    PTVector4u text[64] = { 0 };
};

#define min(a,b) a > b ? b : a

void PTTextNode::updateText(std::string _text)
{
    text = _text;

    TextBuffer buf;
    memcpy((void*)(&buf), text.data(), min(text.size(), 64 * 4 * 4));

    material->setUniform(2, buf);
}

PTTextNode::PTTextNode(PTDeserialiser::ArgMap arguments) : PTNode(arguments)
{
    material = PTResourceManager::get()->createMaterial("res/text.ptmat");
    mesh = PTResourceManager::get()->createMesh("res/obj/plane.obj");

    addDependency(material, false);
    addDependency(mesh, false);

    PTRenderServer::get()->addDrawRequest(this, mesh, material);

    updateText(text);
    updateText("Hello, World!    ABCDEFGHIJKLMNOPQRSTUVWXYZ    abcdefghijklmnopqrstuvwxyz");
}

PTTextNode::~PTTextNode()
{
    PTRenderServer::get()->removeAllDrawRequests(this);

    removeDependency(material);
    removeDependency(mesh);
}

