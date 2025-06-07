#include "application.h"

#include <vector>

#include "input.h"
#include "debug.h"
#include "render_server.h"
#include "scene.h"

using namespace std;

static PTApplication* application = nullptr;

PTApplication::PTApplication(unsigned int _width, unsigned int _height)
{
    width = _width;
    height = _height;
}

void PTApplication::start()
{
    application = this;
    program_start = chrono::high_resolution_clock::now();

    initWindow();
    PTInput::init(window);

	uint32_t glfw_extension_count = 0;
    const char** glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);

    vector<const char*> extensions(glfw_extensions, glfw_extensions + glfw_extension_count);
	PTRenderServer::init(window, extensions);

    current_scene = PTScene_T::createScene("res/demo.ptscn");

    mainLoop();

    current_scene = nullptr;

	PTRenderServer::deinit();
	PTInput::deinit();
    deinitWindow();

    if (!should_stop)
        debugDeinit();
}

PTApplication* PTApplication::get()
{
    return application;
}

void PTApplication::initWindow()
{
    debugLog("initialising glfw...");
    glfwInit();

    debugLog("    initialising window...");
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    window = glfwCreateWindow(width, height, "planetarium", nullptr, nullptr);
    glfwSetFramebufferSizeCallback(window, windowResizeCallback);

    debugLog("done.");
}

void PTApplication::mainLoop()
{
    int frame_total_number = 0;
    last_frame_start = chrono::high_resolution_clock::now();
    while (!glfwWindowShouldClose(window) && !should_stop)
    {
        glfwPollEvents();

        auto now = chrono::high_resolution_clock::now();
        chrono::duration<float> frame_time = now - last_frame_start;

        frame_time_running_mean_us = static_cast<uint32_t>((frame_time_running_mean_us * 0.8f) + (chrono::duration_cast<chrono::microseconds>(frame_time).count() * 0.2f));

        debugFrametiming(frame_time_running_mean_us / 1000.0f, frame_total_number);        
        last_frame_start = now;

        if (PTInput::get()->isKeyDown('R'))
        {
            debugLog("removing scene!");
            current_scene = nullptr;
        }
        else if (PTInput::get()->isKeyDown('L'))
        {
            debugLog("loading scene!");
            current_scene = PTScene_T::createScene("res/demo.ptscn");
        }

        if (wants_new_scene)
        {
            wants_new_scene = false;
            debugLog("loading new scene: " + new_scene_path);
            current_scene = PTScene_T::createScene(new_scene_path);
        }

        if (current_scene != nullptr)
            current_scene->update(frame_time.count());

        PTRenderServer::get()->update();

        frame_total_number++;
    }
}

void PTApplication::deinitWindow()
{
    glfwDestroyWindow(window);

    glfwTerminate();
}

void PTApplication::windowResizeCallback(GLFWwindow* window, int new_width, int new_height)
{
	get()->width = new_width;
	get()->height = new_height;
    PTRenderServer::get()->setWindowResized();
}

float PTApplication::getAspectRatio() const
{
	return (float)width / (float)height;
}

PTVector2u PTApplication::getFramebufferSize()
{
	glfwGetFramebufferSize(window, &width, &height);
	return PTVector2u{ static_cast<uint32_t>(width), static_cast<uint32_t>(height) };
}

void PTApplication::getCameraMatrix(PTMatrix4f& world_to_view, PTMatrix4f& view_to_clip) const
{
	if (current_scene == nullptr)
		return;
	
	current_scene->getCameraMatrix(getAspectRatio(), world_to_view, view_to_clip);
}

PTVector3f PTApplication::getCameraPosition() const
{
    if (current_scene == nullptr)
        return PTVector3f{ 0, 0, 0 };
    if (current_scene->getCamera() == nullptr)
        return PTVector3f{ 0, 0, 0 };

    return current_scene->getCamera()->getTransform()->getPosition();
}

float PTApplication::getTotalTime()
{
	chrono::duration<float> since = chrono::high_resolution_clock::now() - program_start;
	return since.count();
}
