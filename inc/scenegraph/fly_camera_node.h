#pragma once

#include "camera_node.h"

class PTFlyCameraNode_T : public PTCameraNode_T
{
    friend class PTNode_T;
public:
    virtual void process(float delta_time) override;

protected:
	PTFlyCameraNode_T(PTDeserialiser::ArgMap arguments);
};

typedef PTCountedPointer<PTFlyCameraNode_T> PTFlyCameraNode;
