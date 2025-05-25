#pragma once

// the render graph consists of a timeline of instructions - either cameras to draw, or post process steps to run
// each command will have inputs and outputs which read from and write to buffers
// those buffers can then be used as the inputs/outputs of other commands
// one buffer is the framebuffer, and can only be written to once
// cameras will output colour, depth, and extra buffers, and you can choose which you want to keep in buffers
// then any of those can be bound to inputs on post process steps, which can also have multiple outputs

/*
     buf a          buf b        framebuffer
       ^              ^               ^
|      |       |      |       |       |        |
| render cam 1 | render cam 2 | post process 1 |
|              |              |    ^       ^   |
                                   |       |
                                 buf a   buf b
*/

#include <vector>

struct PTRGStep
{
    size_t colour_buffer_binding;
    size_t depth_buffer_binding;
    size_t normal_buffer_binding;
    size_t extra_buffer_binding;
};

struct PTRGCameraStep : public PTRGStep
{
    size_t camera_slot;
};

class PTMaterial;

struct PTRGProcessStep : public PTRGStep
{
    PTMaterial* material;
    std::vector<std::pair<size_t, uint16_t>> input_bindings;
};

class PTRGGraph
{
private:
};