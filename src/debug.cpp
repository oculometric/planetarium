#include "debug.h"

#define STUI_IMPLEMENTATION
#define STUI_KEEP_DEFINES
#include <stui_script.h>
#include <thread>
#include <format>
#include <chrono>

#include "debug_ui.h"

using namespace stui;
using namespace std;

class PropertyView : public Component, public Utility
{
	int last_render_height = 0;

public:
	map<string, string> elements;
    int scroll;
	int selected_index;

    PropertyView(map<string, string> _elements = { }, int _scroll = 0, int _selected_index = 0) : elements(_elements), scroll(_scroll), selected_index(_selected_index) { }

    GETTYPENAME_STUB("PropertyView");

	RENDER_STUB
#ifdef STUI_IMPLEMENTATION
	{
		if (size.x < 2 || size.y < 2) return;
		last_render_height = size.y;
		int row = -1 - scroll;
		int index = -1;
		selected_index = max(min(selected_index, static_cast<int>(elements.size()) - 1), 0);
		for (pair<string, string> element : elements)
		{
			index++;
			row++;
			if (row < 0) continue;
			if (row >= size.y) break;

			if ((row == 0 && index != 0) || (row == size.y - 1 && index != static_cast<int>(elements.size()) - 1))
				output_buffer[row * size.x] = UNICODE_ELLIPSIS_VERTICAL;
			else
			{
				string right_str = stripNullsAndMore(element.second, "\n\t");
				drawText(right_str, Coordinate{ size.x - static_cast<int>(right_str.length()), row }, Coordinate{ size.x, 1 }, output_buffer, size);
				drawText(stripNullsAndMore(element.first, "\n\t") + ":", Coordinate{ 0, row }, Coordinate{ size.x, 1 }, output_buffer, size);
			}
		}
		
		fillColour(focused ? getHighlightedColour() : getUnfocusedColour(), Coordinate{ 0, selected_index - scroll }, Coordinate{ size.x,1 }, output_buffer, size);
	}
#endif
	;

	GETMAXSIZE_STUB { return Coordinate{ -1, -1 }; }
	GETMINSIZE_STUB { return Coordinate{ 10, 3 }; }

	ISFOCUSABLE_STUB { return true; }

	HANDLEINPUT_STUB
#ifdef STUI_IMPLEMENTATION
	{
		if (input_character == Input::ArrowKeys::DOWN && selected_index < static_cast<int>(elements.size()) - 1)
		{
			selected_index++;
			if (selected_index - scroll >= last_render_height - 1 && (static_cast<int>(elements.size()) - scroll > last_render_height))
				scroll++;
		}
		else if (input_character == Input::ArrowKeys::UP && selected_index > 0)
		{
			selected_index--;
			if (selected_index - scroll < 1 && scroll > 0)
				scroll--;
		}
		else return false;

		return true;
	}
#endif
	;

};

class PTDebugManager
{
private:
    Page* main_page = nullptr;
    TextArea* console = nullptr;
    Label* frame_label = nullptr;
    PropertyView* scene_props_box = nullptr;
    PropertyView* object_props_box = nullptr;

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
        scene_props_box = new PropertyView();
        main_page->get<BorderedBox>("scene_box")->child = scene_props_box;
        object_props_box = new PropertyView();
        main_page->get<BorderedBox>("object_box")->child = object_props_box;
        main_page->ensureIntegrity();

        main_page->focusable_component_sequence = { console, scene_props_box, object_props_box };

        render_thread = thread(&PTDebugManager::renderLoop, this);
    }

    void appendToLog(string s);
    void setFrametime(float delta, int number);
    void showExitButton();
    static void exitButtonCallback();

    inline void setSceneProp(string name, string val) { scene_props_box->elements[name] = val; }
    inline void setObjectProp(string name, string val) { object_props_box->elements[name] = val; }
    inline void clearSceneProp(string name) { scene_props_box->elements.erase(name); }
    inline void clearObjectProp(string name) { object_props_box->elements.erase(name); }

    inline ~PTDebugManager()
    {
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
        mgr->showExitButton();
        delete mgr;
    }
}

void debugLog(string text)
{
    if (mgr == nullptr) return;
    auto now = chrono::system_clock::now().time_since_epoch();
	auto hours = chrono::duration_cast<chrono::hours>(now);
	auto minutes = chrono::duration_cast<chrono::minutes>(now - hours);
	auto seconds = chrono::duration_cast<chrono::seconds>(now - hours - minutes);
	auto millis = chrono::duration_cast<chrono::milliseconds>(now - hours - minutes - seconds);
    string str = format("[{:2}:{:2}:{:2}.{:3}]: {}", (int)(hours.count() % 24), (int)minutes.count(), (int)seconds.count(), (int)millis.count(), text);
    mgr->appendToLog(str);
}

void debugFrametiming(float delta_time, int frame_number)
{
    if (mgr == nullptr) return;
    mgr->setFrametime(delta_time, frame_number);
}

void debugSetSceneProperty(std::string name, std::string content)
{
    if (mgr == nullptr) return;
    mgr->setSceneProp(name, content);
}

void debugSetObjectProperty(std::string name, std::string content)
{
    if (mgr == nullptr) return;
    mgr->setObjectProp(name, content);
}

void debugClearSceneProperty(std::string name, std::string content)
{
    if (mgr == nullptr) return;
    mgr->clearSceneProp(name);
}

void debugClearObjectProperty(std::string name, std::string content)
{
    if (mgr == nullptr) return;
    mgr->clearObjectProp(name);
}

void PTDebugManager::exitButtonCallback()
{
    mgr->should_halt = true;
}

void PTDebugManager::appendToLog(string s)
{
    console->text += s + '\n';
    console->scroll++;
    // if (console->text.length() > 5000)
    // {
    //     size_t cutoff = console->text.find('\n', 5000);
    //     if (cutoff == string::npos || cutoff - 5000 > 200)
    //         cutoff = 5000;
    //     console->text = console->text.substr(cutoff);
    // }
}

void PTDebugManager::setFrametime(float delta, int number)
{
    frame_label->text = format("{:.2f}fps | {:.2f}ms ({})", 1000.0f / delta, delta, number);
}

void PTDebugManager::showExitButton()
{
    VerticalBox* vb = (VerticalBox*)((main_page->getRoot()->getAllChildren())[0]->getAllChildren()[0]);
    Label* l = (Label*)(vb->children[3]);
    vb->children[3] = new Button("press enter to exit", exitButtonCallback);
    main_page->focusable_component_sequence.push_back(vb->children[3]);
    main_page->setFocusIndex(main_page->focusable_component_sequence.size() - 1);
    main_page->ensureIntegrity();
}

void PTDebugManager::renderLoop()
{
    while (!should_halt)
    {
        bool dirty = main_page->checkInput();

        //main_page->render();

        main_page->framerate(24);
    }
}
