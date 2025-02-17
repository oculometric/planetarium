# how to create a new node type:

publicly subclass `PTNode`
`friend class PTResourceManager`
implement `PTNewTypeNode(PTDeserialiser::ArgMap arguments)`
implement `~PTNewTypeNode()`
add a new line at the top of deserialiser.cpp `pair<string, PTNodeInstantiateFunc>("NewTypeNode", instantiateNode<PTNewTypeNode>)`