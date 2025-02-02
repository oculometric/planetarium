#include "debug.h"

#define STUI_IMPLEMENTATION
#include <stui_script.h>
#include <thread>
#include <format>

#include "debug_ui.h"

using namespace stui;
using namespace std;

class PTDebugManager
{
private:
    Page* main_page = nullptr;
    TextArea* console = nullptr;
    Label* frame_label = nullptr;
    VerticalBox* scene_props_box = nullptr;
    VerticalBox* object_props_box = nullptr;

    bool should_halt = false;
    thread render_thread;

public:
    inline PTDebugManager()
    {
        Terminal::configure("planetarium");

        LayoutReader reader;
        main_page = reader.readPage(debug_mainpage_sls);
        console = main_page->get<TextArea>("console");
        frame_label = main_page->get<Label>("frame_label");
        scene_props_box = main_page->get<VerticalBox>("scene_props_box");
        object_props_box = main_page->get<VerticalBox>("object_props_box");
        main_page->focusable_component_sequence.push_back(console);

        render_thread = thread(&PTDebugManager::renderLoop, this);
    }

    void appendToLog(string s);
    void setFrametime(float delta, int number);

    inline ~PTDebugManager()
    {
        should_halt = true;
        render_thread.join();

        main_page->render();

        main_page->destroyAllComponents({ });
        delete main_page;

        Terminal::unConfigure(false);

        cout << endl;
    }

private:
    void renderLoop();
};

static PTDebugManager* mgr = nullptr;

void debugInit()
{
    if (mgr == nullptr)
        mgr = new PTDebugManager();
    else
    {
        delete mgr;
        mgr = new PTDebugManager();
    }
}

void debugDeinit()
{
    if (mgr != nullptr)
    {
        delete mgr;
        mgr = nullptr;
    }
}

void debugLog(string text)
{
    if (mgr == nullptr) return;
    mgr->appendToLog(text);
}

void debugFrametiming(float delta_time, int frame_number)
{
    if (mgr == nullptr) return;
    mgr->setFrametime(delta_time, frame_number);
}

void PTDebugManager::appendToLog(string s)
{
    console->text += s + '\n';
    console->scroll++;
    if (console->text.length() > 5000)
    {
        size_t cutoff = console->text.find('\n', 5000);
        if (cutoff == string::npos || cutoff - 5000 > 200)
            cutoff = 5000;
        console->text = console->text.substr(cutoff);
    }
}

void PTDebugManager::setFrametime(float delta, int number)
{
    frame_label->text = format("{:.2f}fps | {:.2f}ms ({})", 1000.0f / delta, delta, number);
}

void PTDebugManager::renderLoop()
{
    while (true)
    {
        bool dirty = main_page->checkInput();

        main_page->render();

        main_page->framerate(48);
    }
}
