#pragma once

#include "math/ptmath.h"

// this simply prevents us from getting a million 'variable defined but not used' errors
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4101)
#elif defined(__GNUG__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
#endif

const uint32_t MAX_FRAMES_IN_FLIGHT = 2;

const uint16_t TRANSFORM_UNIFORM_BINDING = 0;
const uint16_t SCENE_UNIFORM_BINDING = 1;

static const char* DEFAULT_SHADER_PATH = "res/engine/shader/default";
static const char* DEFAULT_MATERIAL_PATH = "res/engine/material/default.ptmat";
static const char* DEFAULT_TEXTURE_PATH = "res/engine/texture/blank.bmp";

#ifdef _MSC_VER
#pragma warning(pop)
#elif defined(__GNUG__)
#pragma GCC diagnostic pop
#endif

struct TransformUniforms
{
    float model_to_world[16];
    float world_to_view[16];
    float view_to_clip[16];
    uint32_t object_id;
};

struct SceneUniforms
{
    PTVector2f viewport_size;
    float time;
};

#include <fstream>
#include <string>
#include <array>
#ifndef _WIN32
#include <unistd.h>
#else
#define popen _popen
#define pclose _pclose
#endif


inline int exec(std::string command, std::string& output)
{
    const size_t buffer_size = 512;
    std::array<char, buffer_size> buffer;

    auto pipe = popen((command + " 2>&1").c_str(), "r");
    if (!pipe)
    {
        output = "popen failed.";
        return -1;
    }

    output = "";
    size_t count;
    do {
        if ((count = fread(buffer.data(), 1, buffer_size, pipe)) > 0)
            output.insert(output.end(), std::begin(buffer), std::next(std::begin(buffer), count));
    } while (count > 0);

    return pclose(pipe);
}
