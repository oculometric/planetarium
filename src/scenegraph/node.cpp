#include "node.h"

PTNode::PTNode(PTDeserialiser::ArgMap arguments)
{
    if (arguments.contains("position") && arguments["position"].type == PTDeserialiser::ArgType::VECTOR3_ARG)
        transform.setLocalPosition(arguments["position"].v3_val);
    if (arguments.contains("rotation") && arguments["rotation"].type == PTDeserialiser::ArgType::VECTOR4_ARG)
        transform.setLocalRotation(arguments["rotation"].v4_val);
    if (arguments.contains("scale") && arguments["scale"].type == PTDeserialiser::ArgType::VECTOR3_ARG)
        transform.setLocalScale(arguments["scale"].v3_val);
}
