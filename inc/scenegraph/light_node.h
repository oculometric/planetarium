#pragma once

#include "node.h"
#include "constant.h"

class PTLightNode : public PTNode
{
    friend class PTResourceManager;
private:
    PTVector3f colour = PTVector3f{ 1.0f, 1.0f, 1.0f };
    float brightness = 1.0f;
    float half_angle = 180.0f;
    bool directional = true;

protected:
    PTLightNode(PTDeserialiser::ArgMap arguments);
    ~PTLightNode();

public:
    inline void setColour(PTVector3f new_colour) { colour = new_colour; }
    inline PTVector3f getColour() const { return colour; }
    inline void setBrightness(float new_brightness) { brightness = new_brightness; }
    inline float getBrightness() const { return brightness; }
    inline void setHalfAngle(float new_half_angle) { half_angle = new_half_angle; }
    inline float getHalfAngle() const { return half_angle; }
    inline void setDirectional(bool new_directional) { directional = new_directional; }
    inline bool getDirectional() const { return directional; }

    LightDescription getDescription();
};