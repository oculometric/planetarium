#pragma once

#include "camera_node.h"

class PTFlyCameraNode : public PTCameraNode
{
	friend class PTResourceManager;
public:
    virtual void process(float delta_time) override;

protected:
	PTFlyCameraNode(PTDeserialiser::ArgMap arguments);
};