#pragma once

#include <set>

class PTResourceManager;

class PTResource
{
    friend class PTResourceManager;
private:
    std::set<PTResource*> dependencies;
    size_t reference_counter = 0;

protected:
    virtual ~PTResource() { };

public:
    void addReferencer();
    void removeReferencer();

    void addDependency(PTResource* resource, bool increment_counter = true);
    void removeDependency(PTResource* resource, bool decrement_counter = true);
};