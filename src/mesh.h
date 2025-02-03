#pragma once

#include <vulkan/vulkan.h>
#include <array>

#include "../lib/oculib/vector3.h"

struct PTVertex
{ // TODO: add normal, tangent, uv
    OLVector3f position;
    OLVector3f colour;
};

class PTMesh
{
public:
    static VkVertexInputBindingDescription getVertexBindingDescription();
    static std::array<VkVertexInputAttributeDescription, 2> getVertexAttributeDescriptions();
};