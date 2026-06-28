#pragma once

#include "Vulkitten/Core/Core.h"

#include <string>
#include <vector>
#include <glm/glm.hpp>

namespace Vulkitten {

class FileSystem;

// ============================================================
// GltfMeshData - cooked mesh data extracted from a glTF file.
// ============================================================

struct GltfVertex
{
    glm::vec3 Position;
    glm::vec3 Normal;
    glm::vec2 TexCoord;
};

struct GltfMeshData
{
    std::string Name;
    std::vector<GltfVertex> Vertices;
    std::vector<uint32_t>   Indices;
    int32_t MaterialIndex = -1;
};

// ============================================================
// GltfLoader - loads glTF 2.0 files via tinygltf.
//
// Wraps tinygltf to provide a simpler API for extracting mesh
// data for rendering. Handles virtual path resolution through
// the engine FileSystem.
// ============================================================

class VKT_API GltfLoader
{
public:
    explicit GltfLoader(FileSystem& fileSystem);
    ~GltfLoader() = default;

    // Load a glTF file from a virtual path (e.g. "sandbox://...")
    // Returns the number of meshes loaded, or 0 on failure.
    uint32_t Load(const std::string& virtualPath);

    // Access loaded mesh data.
    const std::vector<GltfMeshData>& GetMeshes() const { return m_Meshes; }

    // Clear all loaded data.
    void Clear();

    // Get the last error message (empty if no error).
    const std::string& GetLastError() const { return m_LastError; }

private:
    FileSystem& m_FileSystem;
    std::vector<GltfMeshData> m_Meshes;
    std::string m_LastError;
};

} // namespace Vulkitten
