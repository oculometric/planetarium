#include "scene.h"

#include <iostream>
#include "application.h"

using namespace std;

PTScene::PTScene(PTApplication* application)
{
    owner = application;
}

void PTScene::update(float delta_time)
{
    cout << "update! dt: " << delta_time << endl;
    cout << "x move value: " << owner->getInputManager()->getAxisState(PTInputAxis::MOVE_AXIS_X) << endl;
}

OLMatrix4f PTScene::getCameraMatrix()
{
    return ~camera.getLocalTransform() * camera.getProjectionMatrix();
}