#include "resource_manager.h"

#include <fstream>

#include "mesh.h"

#include "debug.h"
#include "scene.h"
#include "image.h"
#include "shader.h"
#include "material.h"
#include "render_server.h"

using namespace std;

static PTResourceManager* resource_manager = nullptr;

void PTResourceManager::init(VkDevice _device, PTPhysicalDevice& _physical_device)
{
    if (resource_manager != nullptr)
        return;
    
    resource_manager = new PTResourceManager(_device, _physical_device);
}

void PTResourceManager::deinit()
{
    if (resource_manager == nullptr)
        return;
    
    delete resource_manager;
    resource_manager = nullptr;
}

PTResourceManager* PTResourceManager::get()
{
    return resource_manager;
}

//PTScene* PTResourceManager::createScene(string file_name, bool force_duplicate)
//{
//    string identifier = "scene-" + file_name;
//    PTScene* scene = nullptr;
//
//    if (!force_duplicate)
//        scene = tryGetExistingResource<PTScene>(identifier);
//    if (scene == nullptr)
//    {
//        ifstream file(file_name, ios::ate);
//        if (!file.is_open())
//            return nullptr;
//
//        size_t size = file.tellg();
//        string scene_text;
//        scene_text.resize(size, ' ');
//        file.seekg(0);
//        file.read(scene_text.data(), size);
//
//        scene = new PTScene();
//        PTDeserialiser::deserialiseScene(scene, scene_text);
//
//        resources.emplace(identifier, scene);
//    }
//
//    scene->addReferencer();
//
//    return scene;
//}

PTResourceManager::~PTResourceManager()
{
    debugLog("shutting down resource manager.");

    /*if (resources.empty())
    {
        debugLog("well done for cleaning up!");
        return;
    }

    debugLog("resource manager cleaning up " + to_string(resources.size()) + " lingering resources...");*/

    /*for (auto current_pair : resources)
    {
        PTResource* current = current_pair.second;
        current->reference_counter = 0;
        for (auto target_pair : resources)
        {
            if (target_pair.second == current)
                continue;
            if (target_pair.second->dependencies.contains(current))
                current->reference_counter++;
        }
    }

    while (!resources.empty())
    {
        auto iter = resources.begin();
        auto end = resources.end();
        while (iter != end && (*iter).second->reference_counter != 0)
            iter++;

        if (iter == end)
            break;
        
        debugLog("    unloading resource " + (*iter).first);
        PTResource* res = (*iter).second;
        resources.erase(iter);
        delete res;
    }

    if (!resources.empty())
    {
        debugLog("    WARNING: cycle detected in resource dependency graph! unloads of the last " + to_string(resources.size()) + " resources may be out of order");
        for (auto pair : resources)
        {
            debugLog("    unloading resource " + pair.first);
            delete pair.second;
        }
    }*/
    //resources.clear();

    debugLog("done.");
}
