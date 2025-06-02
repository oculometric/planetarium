#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>
#include <chrono>

#include "math/ptmath.h"

#ifdef _WIN32
typedef std::chrono::steady_clock clocktype;
#else
typedef std::chrono::system_clock clocktype;
#endif

class PTScene;

class PTApplication
{
public:
    bool should_stop = false;

private:
    int width;
    int height;

    GLFWwindow* window = nullptr;

    clocktype::time_point last_frame_start;
    clocktype::time_point program_start;
    uint32_t frame_time_running_mean_us;

    PTScene* current_scene = nullptr;
    bool wants_new_scene = false;
    std::string new_scene_path = "";
public:
    PTApplication(unsigned int _width, unsigned int _height);

    PTApplication() = delete;
    PTApplication(PTApplication& other) = delete;
    PTApplication(PTApplication&& other) = delete;
    void operator=(PTApplication& other) = delete;
    void operator=(PTApplication&& other) = delete;

    void start();

    static PTApplication* get();

    float getAspectRatio() const;
	PTVector2u getFramebufferSize();

	void getCameraMatrix(PTMatrix4f& world_to_view, PTMatrix4f& view_to_clip) const;
    PTVector3f getCameraPosition() const;

	float getTotalTime();

    inline void openScene(std::string path) { wants_new_scene = true; new_scene_path = path; }

private:
    void initWindow();
    void mainLoop();
    void deinitWindow();

    static void windowResizeCallback(GLFWwindow* window, int new_width, int new_height);
};