#include "mesh.h"

using namespace std;

VkVertexInputBindingDescription PTMesh::getVertexBindingDescription()
{
    VkVertexInputBindingDescription description{ };
    description.binding = 0;
    description.stride = sizeof(PTVertex);
    description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    return description;
}

array<VkVertexInputAttributeDescription, 2> PTMesh::getVertexAttributeDescriptions()
{
    array<VkVertexInputAttributeDescription, 2> descriptions{ };

    descriptions[0].binding = 0;
    descriptions[0].location = 0;
    descriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    descriptions[0].offset = offsetof(PTVertex, position);

    descriptions[1].binding = 0;
    descriptions[1].location = 1;
    descriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    descriptions[1].offset = offsetof(PTVertex, colour);

    return descriptions;
}
