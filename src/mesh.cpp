#include "mesh.h"

#include <cstring>
#include <fstream>
#include <stdexcept>

#include "../lib/oculib/matrix3.h"
#include "resource_manager.h"

using namespace std;

VkVertexInputBindingDescription PTMesh::getVertexBindingDescription()
{
    VkVertexInputBindingDescription description{ };
    description.binding = 0;
    description.stride = sizeof(PTVertex);
    description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    return description;
}

array<VkVertexInputAttributeDescription, 5> PTMesh::getVertexAttributeDescriptions()
{
    array<VkVertexInputAttributeDescription, 5> descriptions{ };

    descriptions[0].binding = 0;
    descriptions[0].location = 0;
    descriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    descriptions[0].offset = offsetof(PTVertex, position);

    descriptions[1].binding = 0;
    descriptions[1].location = 1;
    descriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    descriptions[1].offset = offsetof(PTVertex, colour);

    descriptions[2].binding = 0;
    descriptions[2].location = 2;
    descriptions[2].format = VK_FORMAT_R32G32B32_SFLOAT;
    descriptions[2].offset = offsetof(PTVertex, normal);

    descriptions[3].binding = 0;
    descriptions[3].location = 3;
    descriptions[3].format = VK_FORMAT_R32G32B32_SFLOAT;
    descriptions[3].offset = offsetof(PTVertex, tangent);

    descriptions[4].binding = 0;
    descriptions[4].location = 4;
    descriptions[4].format = VK_FORMAT_R32G32_SFLOAT;
    descriptions[4].offset = offsetof(PTVertex, uv);

    return descriptions;
}

PTMesh::PTMesh(VkDevice _device, const PTPhysicalDevice& physical_device, std::string file_name)
{
    device = _device;

    vector<PTVertex> verts;
    vector<uint16_t> inds;

    readFileToBuffers(file_name, verts, inds);
    createVertexBuffers(physical_device, verts, inds);
}

PTMesh::PTMesh(VkDevice _device, const PTPhysicalDevice& physical_device, std::vector<PTVertex> vertices, std::vector<uint16_t> indices)
{
    device = _device;

    createVertexBuffers(physical_device, vertices, indices);
}

PTMesh::~PTMesh()
{
    // TODO: tell the resource manager to release them as well, after removing them as dependencies
    removeDependency(index_buffer);
    removeDependency(vertex_buffer);
}

struct PTFaceCorner { uint16_t co; uint16_t uv; uint16_t vn; };

// splits a formatted OBJ face corner into its component indices
static inline PTFaceCorner splitOBJFaceCorner(string str)
{
    PTFaceCorner fci = { 0,0,0 };
    size_t first_break_ind = str.find('/');
    if (first_break_ind == string::npos) return fci;
    fci.co = static_cast<uint16_t>(stoi(str.substr(0, first_break_ind)) - 1);
    size_t second_break_ind = str.find('/', first_break_ind + 1);
    if (second_break_ind != first_break_ind + 1)
        fci.uv = static_cast<uint16_t>(stoi(str.substr(first_break_ind + 1, second_break_ind - first_break_ind)) - 1);
    fci.vn = static_cast<uint16_t>(stoi(str.substr(second_break_ind + 1, str.find('/', second_break_ind + 1) - second_break_ind)) - 1);

    return fci;
}

struct PTFaceCornerReference
{
    uint16_t normal_index;
    uint16_t uv_index;
    uint16_t transferred_vert_index;
};

static pair<OLVector3f, OLVector3f> computeTangent(OLVector3f co_a, OLVector3f co_b, OLVector3f co_c, OLVector2f uv_a, OLVector2f uv_b, OLVector2f uv_c)
{
    // vector from the target vertex to the second vertex
    OLVector3f ab = OLVector3f(co_b.x - co_a.x, co_b.y - co_a.y, co_b.z - co_a.z); ab = OLVector3f(ab.x, ab.y, ab.z);
    // vector from the target vertex to the third vertex
    OLVector3f ac = OLVector3f(co_c.x - co_a.x, co_c.y - co_a.y, co_c.z - co_a.z); ac = OLVector3f(ac.x, ac.y, ac.z);
    // delta uv between target and second
    OLVector2f uv_ab = OLVector2f(uv_b.x - uv_a.x, uv_b.y - uv_a.y); uv_ab = OLVector2f(uv_ab.x, uv_ab.y);
    // delta uv between target and third
    OLVector2f uv_ac = OLVector2f(uv_c.x - uv_a.x, uv_c.y - uv_a.y); uv_ac = OLVector2f(uv_ac.x, uv_ac.y);
    // matrix representing UVs
    OLMatrix3f uv_mat = OLMatrix3f
    (
        uv_ab.x, uv_ac.x, 0,
        uv_ab.y, uv_ac.y, 0,
        0,       0,       1
    );
    // matrix representing vectors between vertices
    OLMatrix3f vec_mat = OLMatrix3f
    (
        ab.x, ac.x, 0,
        ab.y, ac.y, 0,
        ab.z, ac.z, 0
    );
    
    // we should be able to express the vectors from A->B and A->C with reference to the difference in UV coordinate and the tangent and bitangent:
    //
    // AB = (duv_ab.x * T) + (duv_ab.y * B)
    // AC = (duv_ac.x * T) + (duv_ac.y * B)
    // 
    // this gives us 6 simultaneous equations for the XYZ coordinates of the tangent and bitangent
    // these can be expressed and solved with matrices:
    // 
    // [ AB.x  AC.x  0 ]     [ T.x  B.x  N.x ]   [ duv_ab.x  duv_ac.x  0 ]
    // [ AB.y  AC.y  0 ]  =  [ T.y  B.y  N.y ] * [ duv_ab.y  duv_ac.y  0 ]
    // [ AB.z  AC.z  0 ]     [ T.z  B.z  N.z ]   [ 0         0         1 ]
    //

    vec_mat =  vec_mat * ~uv_mat;

    pair<OLVector3f, OLVector3f> ret;
    ret.first = norm(vec_mat.col0());                 // extract tangent

    return ret;
}

void PTMesh::readFileToBuffers(std::string file_name, std::vector<PTVertex>& vertices, std::vector<uint16_t>& indices)
{
    ifstream file;
    file.open(file_name);
    if (!file.is_open())
        throw runtime_error("unable to open file " + file_name);

    // vectors to load data into
    vector<OLVector3f> tmp_co;
    vector<PTFaceCorner> tmp_fc;
    vector<OLVector2f> tmp_uv;
    vector<OLVector3f> tmp_vn;

    // temporary locations for reading data to
    string tmps;
    OLVector3f tmp3;
    OLVector2f tmp2;

    // repeat for every line in the file
    while (!file.eof())
    {
        file >> tmps;
        if (tmps == "v")
        {
            // read a vertex coordinate
            file >> tmp3.x;
            file >> tmp3.y;
            file >> tmp3.z;
            tmp_co.push_back(tmp3);
        }
        else if (tmps == "vn")
        {
            // read a face corner normal
            file >> tmp3.x;
            file >> tmp3.y;
            file >> tmp3.z;
            tmp_vn.push_back(tmp3);
        }
        else if (tmps == "vt")
        {
            // read a face corner uv (texture coordinate)
            file >> tmp2.x;
            file >> tmp2.y;
            tmp_uv.push_back(tmp2);
        }
        else if (tmps == "f")
        {
            // read a face (only supports triangles)
            file >> tmps;
            tmp_fc.push_back(splitOBJFaceCorner(tmps));
            file >> tmps;
            tmp_fc.push_back(splitOBJFaceCorner(tmps));
            file >> tmps;
            tmp_fc.push_back(splitOBJFaceCorner(tmps));

            swap(tmp_fc[tmp_fc.size() - 1], tmp_fc[tmp_fc.size() - 3]);
        }
        file.ignore(SIZE_MAX, '\n');
    }

    // swap the first and last face corner of each triangle, to flip the face order
    for (uint32_t i = 0; i < tmp_fc.size() - 2; i += 3)
    {
        PTFaceCorner fc_i = tmp_fc[i];
        tmp_fc[i] = tmp_fc[i+2];
        tmp_fc[i+2] = fc_i;
    }

    // for each coordinate, stores a list of all the times it has been used by a face corner, and what the normal/uv index was for that face corner
    // this allows us to tell when we should split a vertex (i.e. if it has already been used by another face corner but which had a different normal and/or a different uv)
    vector<vector<PTFaceCornerReference>> fc_normal_uses(tmp_co.size(), vector<PTFaceCornerReference>());

    vertices.clear();
    indices.clear();

    for (PTFaceCorner fc : tmp_fc)
    {
        bool found_matching_vertex = false;
        uint16_t match = 0;
        for (PTFaceCornerReference existing : fc_normal_uses[fc.co])
        {
            if (existing.normal_index == fc.vn && existing.uv_index == fc.uv)
            {
                found_matching_vertex = true;
                match = existing.transferred_vert_index;
                break;
            }
        }

        if (found_matching_vertex)
        {
            indices.push_back(match);
        }
        else
        {
            PTVertex new_vert;
            new_vert.position = tmp_co[fc.co];
            new_vert.normal = tmp_vn[fc.vn];
            if (tmp_uv.size() > fc.uv)
                new_vert.uv = tmp_uv[fc.uv];

            uint16_t new_index = static_cast<uint16_t>(vertices.size());
            fc_normal_uses[fc.co].push_back(PTFaceCornerReference{ fc.vn, fc.uv, new_index });

            indices.push_back(new_index);
            vertices.push_back(new_vert);
        }
    }

    // compute tangents
    vector<bool> touched = vector<bool>(vertices.size(), false);
    for (uint32_t tri = 0; tri < indices.size() / 3; tri++)
    {
        uint16_t v0 = indices[(tri * 3) + 0]; PTVertex f0 = vertices[v0];
        uint16_t v1 = indices[(tri * 3) + 1]; PTVertex f1 = vertices[v1];
        uint16_t v2 = indices[(tri * 3) + 2]; PTVertex f2 = vertices[v2];
        
        if (!touched[v0]) vertices[v0].tangent = computeTangent(f0.position, f1.position, f2.position, f0.uv, f1.uv, f2.uv).first;
        if (!touched[v1]) vertices[v1].tangent = computeTangent(f1.position, f0.position, f2.position, f1.uv, f0.uv, f2.uv).first;
        if (!touched[v2]) vertices[v2].tangent = computeTangent(f2.position, f0.position, f1.position, f2.uv, f0.uv, f1.uv).first;

        touched[v0] = true; touched[v1] = true; touched[v2] = true;
    }

    // transform from Z back Y up space into Z up Y forward space
    for (PTVertex& fv : vertices)
    {
        fv.position = OLVector3f(fv.position.x, -fv.position.z, fv.position.y);
        fv.normal = OLVector3f(fv.normal.x, -fv.normal.z, fv.normal.y);
        fv.tangent = OLVector3f(fv.tangent.x, -fv.tangent.z, fv.tangent.y);
    }
}

void PTMesh::createVertexBuffers(const PTPhysicalDevice& physical_device, std::vector<PTVertex> vertices, std::vector<uint16_t> indices)
{
    // vertex buffer creation (via staging buffer)
    VkDeviceSize size = sizeof(PTVertex) * vertices.size();
    PTBuffer* staging_buffer = PTResourceManager::get()->createBuffer(device, physical_device, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    
    void* vertex_data = staging_buffer->map();
    memcpy(vertex_data, vertices.data(), (size_t)size);
    staging_buffer->unmap();
    vertex_buffer = PTResourceManager::get()->createBuffer(device, physical_device, size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    staging_buffer->copyTo(vertex_buffer, size);

    staging_buffer->removeReferencer();
    
    // index buffer creation (via staging buffer)
    size = sizeof(uint16_t) * indices.size();
    staging_buffer = PTResourceManager::get()->createBuffer(device, physical_device, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    
    void* index_data = staging_buffer->map();
    memcpy(index_data, indices.data(), (size_t)size);
    staging_buffer->unmap();
    index_buffer = PTResourceManager::get()->createBuffer(device, physical_device, size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    staging_buffer->copyTo(index_buffer, size);

    index_count = indices.size();

    addDependency(vertex_buffer, false);
    addDependency(index_buffer, false);

    staging_buffer->removeReferencer();
}