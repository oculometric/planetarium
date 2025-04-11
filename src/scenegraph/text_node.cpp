#include "text_node.h"

#include "mesh.h"
#include "material.h"
#include "resource_manager.h"
#include "render_server.h"

struct TextBuffer
{
    uint32_t text[256];
};

PTTextNode::PTTextNode(PTDeserialiser::ArgMap arguments) : PTNode(arguments)
{
    material = PTResourceManager::get()->createMaterial("res/text.ptmat");
    mesh = PTResourceManager::get()->createMesh("res/obj/plane.obj");

    addDependency(material, false);
    addDependency(mesh, false);

    PTRenderServer::get()->addDrawRequest(this, mesh, material);

    TextBuffer text;
    text.text[0] = 1;
    text.text[1] = 1;
    text.text[2] = 0;
    text.text[3] = 0;
    text.text[3] = 4;
    text.text[3] = 2;
    text.text[3] = 2;
    text.text[3] = 4;

    material->setUniform(2, text);
}

PTTextNode::~PTTextNode()
{
    PTRenderServer::get()->removeAllDrawRequests(this);

    removeDependency(material);
    removeDependency(mesh);
}

