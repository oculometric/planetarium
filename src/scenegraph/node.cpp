#include "node.h"

#include "application.h"

PTNode_T::~PTNode_T()
{
    scene = nullptr;
}

PTScene_T* PTNode_T::getScene() const { return scene; }

PTApplication* PTNode_T::getApplication() { return PTApplication::get(); }

PTNode_T::PTNode_T(PTDeserialiser::ArgMap arguments)
{
    // read basic transform arguments and apply them
    if (hasArg(arguments, "position", PTDeserialiser::ArgType::VECTOR3_ARG))
        transform.setLocalPosition(arguments["position"].v3_val);
    if (hasArg(arguments, "rotation", PTDeserialiser::ArgType::VECTOR4_ARG))
        transform.setLocalRotation(arguments["rotation"].v4_val);
    if (hasArg(arguments, "scale", PTDeserialiser::ArgType::VECTOR3_ARG))
        transform.setLocalScale(arguments["scale"].v3_val);
}
