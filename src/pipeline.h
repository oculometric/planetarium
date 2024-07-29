#pragma once

#include <vulkan/vulkan.h>

#include "shader.h"

// TODO: move more stuff into the pipeline class. make all of this more object-orientated
struct PTPipeline
{
    VkPipelineLayout layout;
    VkPipeline pipeline;
};