#pragma once

#include "../lib/oculib/camera.h"

class PTScene
{
private:
    OLCamera camera;

public:
    PTScene();
    
    PTScene(PTScene& other) = delete;
    PTScene(PTScene&& other) = delete;
    PTScene operator=(PTScene& other) = delete;
    PTScene operator=(PTScene&& other) = delete;

    virtual void update(float delta_time);

    OLMatrix4f getCameraMatrix();

private:
};