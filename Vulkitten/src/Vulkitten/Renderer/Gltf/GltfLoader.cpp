#include "vktpch.h"
#include "GltfLoader.h"

#include "Vulkitten/Core/FileSystem.h"

// TINYGLTF_IMPLEMENTATION pulls in tinygltf function bodies.
// STB_IMAGE_IMPLEMENTATION is NOT defined here - Vulkitten already
// provides it via OpenGLTexture.cpp.
// STB_IMAGE_WRITE_IMPLEMENTATION is defined here since Vulkitten
// doesn't include stb_image_write elsewhere.
#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <tiny_gltf.h>

namespace Vulkitten {

GltfLoader::GltfLoader(FileSystem& fileSystem)
    : m_FileSystem(fileSystem)
{
}

uint32_t GltfLoader::Load(const std::string& virtualPath)
{
    Clear();

    // Resolve virtual path to physical path.
    std::string physicalPath = m_FileSystem.Resolve(virtualPath);
    if (physicalPath.empty())
    {
        m_LastError = "Failed to resolve path: " + virtualPath;
        VKT_CORE_ERROR("GltfLoader: {0}", m_LastError);
        return 0;
    }

    tinygltf::Model model;
    tinygltf::TinyGLTF loader;
    std::string err, warn;

    // Detect file type from extension
    bool success = false;
    std::string ext = physicalPath.substr(physicalPath.find_last_of('.') + 1);
    if (ext == "glb")
        success = loader.LoadBinaryFromFile(&model, &err, &warn, physicalPath);
    else
        success = loader.LoadASCIIFromFile(&model, &err, &warn, physicalPath);

    if (!warn.empty())
        VKT_CORE_WARN("GltfLoader: {0}", warn);

    if (!err.empty())
        VKT_CORE_ERROR("GltfLoader: {0}", err);

    if (!success)
    {
        m_LastError = "Failed to load glTF: " + physicalPath + " - " + err;
        VKT_CORE_ERROR("GltfLoader: {0}", m_LastError);
        return 0;
    }

    // Extract mesh data
    for (const auto& gltfMesh : model.meshes)
    {
        for (const auto& primitive : gltfMesh.primitives)
        {
            GltfMeshData meshData;
            meshData.Name = gltfMesh.name;
            meshData.MaterialIndex = primitive.material;

            // --- Extract position data (required) ---
            {
                const auto& posAccessor = model.accessors[primitive.attributes.at("POSITION")];
                const auto& posView = model.bufferViews[posAccessor.bufferView];
                const float* posData = reinterpret_cast<const float*>(
                    &model.buffers[posView.buffer].data[posView.byteOffset + posAccessor.byteOffset]);

                // --- Extract normal data (optional) ---
                const float* normalData = nullptr;
                int normalStride = 0;
                auto normalIt = primitive.attributes.find("NORMAL");
                if (normalIt != primitive.attributes.end())
                {
                    const auto& nrmAccessor = model.accessors[normalIt->second];
                    const auto& nrmView = model.bufferViews[nrmAccessor.bufferView];
                    normalData = reinterpret_cast<const float*>(
                        &model.buffers[nrmView.buffer].data[nrmView.byteOffset + nrmAccessor.byteOffset]);
                    normalStride = nrmAccessor.ByteStride(
                        model.bufferViews[nrmAccessor.bufferView]) / sizeof(float);
                }

                // --- Extract texcoord data (optional) ---
                const float* uvData = nullptr;
                int uvStride = 0;
                auto uvIt = primitive.attributes.find("TEXCOORD_0");
                if (uvIt != primitive.attributes.end())
                {
                    const auto& uvAccessor = model.accessors[uvIt->second];
                    const auto& uvView = model.bufferViews[uvAccessor.bufferView];
                    uvData = reinterpret_cast<const float*>(
                        &model.buffers[uvView.buffer].data[uvView.byteOffset + uvAccessor.byteOffset]);
                    uvStride = uvAccessor.ByteStride(
                        model.bufferViews[uvAccessor.bufferView]) / sizeof(float);
                }

                // --- Build vertex buffer ---
                size_t vertexCount = posAccessor.count;
                meshData.Vertices.reserve(vertexCount);
                for (size_t i = 0; i < vertexCount; i++)
                {
                    GltfVertex v;
                    v.Position = glm::vec3(posData[i * 3 + 0], posData[i * 3 + 1], posData[i * 3 + 2]);

                    if (normalData)
                        v.Normal = glm::vec3(
                            normalData[i * (normalStride ? normalStride : 3) + 0],
                            normalData[i * (normalStride ? normalStride : 3) + 1],
                            normalData[i * (normalStride ? normalStride : 3) + 2]);
                    else
                        v.Normal = glm::vec3(0.0f, 1.0f, 0.0f);

                    if (uvData)
                        v.TexCoord = glm::vec2(
                            uvData[i * (uvStride ? uvStride : 2) + 0],
                            uvData[i * (uvStride ? uvStride : 2) + 1]);
                    else
                        v.TexCoord = glm::vec2(0.0f, 0.0f);

                    meshData.Vertices.push_back(v);
                }
            }

            // --- Extract index data ---
            if (primitive.indices >= 0)
            {
                const auto& idxAccessor = model.accessors[primitive.indices];
                const auto& idxView = model.bufferViews[idxAccessor.bufferView];

                meshData.Indices.reserve(idxAccessor.count);
                const uint8_t* idxData = &model.buffers[idxView.buffer].data[idxView.byteOffset + idxAccessor.byteOffset];

                for (size_t i = 0; i < idxAccessor.count; i++)
                {
                    uint32_t index = 0;
                    switch (idxAccessor.componentType)
                    {
                        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
                            index = reinterpret_cast<const uint16_t*>(idxData)[i];
                            break;
                        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
                            index = reinterpret_cast<const uint32_t*>(idxData)[i];
                            break;
                        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
                            index = idxData[i];
                            break;
                        default:
                            continue;
                    }
                    meshData.Indices.push_back(index);
                }
            }

            m_Meshes.push_back(std::move(meshData));
        }
    }

    VKT_CORE_INFO("GltfLoader: loaded {0} mesh(es) from {1}", m_Meshes.size(), virtualPath);
    return static_cast<uint32_t>(m_Meshes.size());
}

void GltfLoader::Clear()
{
    m_Meshes.clear();
    m_LastError.clear();
}

} // namespace Vulkitten
