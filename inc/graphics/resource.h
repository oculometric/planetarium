#pragma once

//#include <map>
#include "reference_counter.h"

class PTResource_T
{
public:
    PTResource_T() = delete;
    PTResource_T(const PTResource_T& other) = delete;
    PTResource_T(const PTResource_T&& other) = delete;
    PTResource_T operator=(const PTResource_T& other) = delete;
    PTResource_T operator=(const PTResource_T&& other) = delete;
    inline ~PTResource_T() { }
};

typedef PTCountedPointer<PTResource_T> PTResource;

//class PTResourceManager;
//
//class PTResource
//{
//    friend class PTResourceManager;
//private:
//    std::map<PTResource*, int> dependencies;
//    size_t reference_counter = 0;
//
//protected:
//    virtual ~PTResource() { };
//
//public:
//    void addReferencer();
//    void removeReferencer();
//
//    void addDependency(PTResource* resource, bool increment_counter = true);
//    void removeDependency(PTResource* resource, bool decrement_counter = true);
//};