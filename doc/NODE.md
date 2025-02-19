# how to create a new node type:

create a new file inside inc/scenegraph
publicly subclass `PTNode`
`friend class PTResourceManager`
implement `PTNewTypeNode(PTDeserialiser::ArgMap arguments)`
implement `~PTNewTypeNode()`