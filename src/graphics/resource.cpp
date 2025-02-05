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
    if (reference_counter == 0)
    {
        debugLog("WARNING: attempt to remove nonexistent referencer!");
        PTResourceManager::get()->releaseResource(this);
    }
    else
    {
        reference_counter--;
        if (reference_counter == 0)
            PTResourceManager::get()->releaseResource(this);
    }
}

void PTResource::addDependency(PTResource* resource, bool increment_counter)
{
    if (dependencies.contains(resource))
        return;
    else
    {
        dependencies.insert(resource);
        if (increment_counter)
            resource->addReferencer();
    }
}

void PTResource::removeDependency(PTResource* resource, bool decrement_counter)
{
    if (dependencies.contains(resource))
    {
        dependencies.erase(resource);
        if (decrement_counter)
            resource->removeReferencer();
    }
    else
        debugLog("WARNING: attempt to remove nonexistent dependency!");
}
