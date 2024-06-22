#pragma once

#include <vulkan/vulkan.h>

#include "shader.h"

struct PTPipeline
{
    VkPipelineLayout layout;
    VkPipeline pipeline;
};