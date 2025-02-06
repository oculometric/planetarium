#pragma once

#include <map>

#include "node.h"
#include "camera.h"
#include "resource.h"

class PTScene : PTResource
{
private:
    // TODO: use this to keep track of named resources we know about
    std::map<std::string, PTResource*> referenced_resources;
    // TODO: always create this. objects should be resources too (allocated via the resource manager, but there should be a scene instantiate function. also, should use a unified constructor and an arugment map for simplicity), so they can use the referencing system. also the scene references the objects
    PTNode* root = nullptr;
    PTCameraNode* camera = nullptr;

public:
    PTScene(PTScene& other) = delete;
    PTScene(PTScene&& other) = delete;
    PTScene operator=(PTScene& other) = delete;
    PTScene operator=(PTScene&& other) = delete;

    template<class T>
    T* instantiate(std::map<std::string, PTDeserialiser::Argument> arguments = { });

    virtual void update(float delta_time);

    PTMatrix4f getCameraMatrix();

private:
// TODO: both of these
    PTScene();

    ~PTScene();
};