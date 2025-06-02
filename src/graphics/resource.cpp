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
    // if we already have the resource, don't add it again!
    if (dependencies.contains(resource))
    {
        if (increment_counter)
            resource->addReferencer();
        return;
    }
    else
    {
        // add to the deps list, then maybe increment its reference counter
        dependencies.insert(resource);
        if (increment_counter)
            resource->addReferencer();
    }
}

void PTResource::removeDependency(PTResource* resource, bool decrement_counter)
{
    if (dependencies.contains(resource))
    {
        // remove from the deps list, then maybe decrement its reference counter
        dependencies.erase(resource);
        if (decrement_counter)
            resource->removeReferencer();
    }
    else // if the resource isn't in the deps list, something has gone horribly wrong
        debugLog("WARNING: attempt to remove nonexistent dependency!");
}
