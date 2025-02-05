#pragma once

#include "camera.h"

class PTScene
{
private:
    PTCamera camera;

public:
    PTScene();
    
    PTScene(PTScene& other) = delete;
    PTScene(PTScene&& other) = delete;
    PTScene operator=(PTScene& other) = delete;
    PTScene operator=(PTScene&& other) = delete;

    virtual void update(float delta_time);

    PTMatrix4f getCameraMatrix();

private:
};