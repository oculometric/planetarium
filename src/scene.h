#pragma once

#include "../lib/oculib/camera.h"
#include "defs.h"

class PTScene
{
private:
    OLCamera camera;
    PTApplication* owner;

public:
    PTScene(PTApplication* application);
    
    PTScene(PTScene& other) = delete;
    PTScene(PTScene&& other) = delete;
    PTScene operator=(PTScene& other) = delete;
    PTScene operator=(PTScene&& other) = delete;

    virtual void update(float delta_time);

    OLMatrix4f getCameraMatrix();

private:
};