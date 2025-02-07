#include "node.h"

#include "application.h"

PTNode::PTNode(PTDeserialiser::ArgMap arguments)
{
    if (hasArg(arguments, "position", PTDeserialiser::ArgType::VECTOR3_ARG))
        transform.setLocalPosition(arguments["position"].v3_val);
    if (hasArg(arguments, "rotation", PTDeserialiser::ArgType::VECTOR4_ARG))
        transform.setLocalRotation(arguments["rotation"].v4_val);
    if (hasArg(arguments, "scale", PTDeserialiser::ArgType::VECTOR3_ARG))
        transform.setLocalScale(arguments["scale"].v3_val);
}

PTApplication* PTNode::getApplication()
{
    return PTApplication::get();
}
