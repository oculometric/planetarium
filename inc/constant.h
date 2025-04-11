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

static const char* DEFAULT_SHADER_PATH = "shr/demo";

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

inline int exec(std::string command, std::string& output)
{
    int status = std::system((command + " > tmp.txt").c_str());
    output = "";
    
    std::ifstream file("tmp.txt", std::ios::ate);
    size_t size = file.tellg();
    output.resize(size, ' ');
    file.seekg(0);
    file.read(output.data(), size);
    file.close();

    return status;
}
