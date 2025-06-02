#include "resource.h"

#include "debug.h"
#include "resource_manager.h"

using namespace std;

void PTResource::addReferencer()
{
    reference_counter++;
}

void PTResource::removeReferencer()
{
    if (reference_counter <= 0)
    {
        // if the reference counter is already zero, something has gone horribly wrong
        debugLog("WARNING: attempt to remove nonexistent referencer!");
        PTResourceManager::get()->releaseResource(this);
    }
    else
    {
        // decrement the reference counter, release ourselves if we've reached zero
        reference_counter--;
        if (reference_counter <= 0)
            PTResourceManager::get()->releaseResource(this);
    }
}

void PTResource::addDependency(PTResource* resource, bool increment_counter)
{
    auto it = dependencies.find(resource);
    // if we already have the resource, don't add it again!
    if (it != dependencies.end())
    {
        it->second = it->second + 1;
        if (increment_counter)
            resource->addReferencer();
        return;
    }
    else
    {
        // add to the deps list, then maybe increment its reference counter
        dependencies[resource] = 1;
        if (increment_counter)
            resource->addReferencer();
    }
}

void PTResource::removeDependency(PTResource* resource, bool decrement_counter)
{
    auto it = dependencies.find(resource);
    if (it != dependencies.end())
    {
        // remove from the deps list, then maybe decrement its reference counter
        it->second = it->second - 1;
        if (it->second == 0)
            dependencies.erase(it);
        if (decrement_counter)
            resource->removeReferencer();
    }
    else // if the resource isn't in the deps list, something has gone horribly wrong
        debugLog("WARNING: attempt to remove nonexistent dependency!");
}
