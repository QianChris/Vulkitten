#pragma once

#include "Vulkitten/Core/Core.h"

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <filesystem>

namespace Vulkitten {

class FileSystem;

// ============================================================
// ShaderData — preprocessed shader source and metadata
// ============================================================

struct ShaderData
{
    std::string VirtualPath;       // Original virtual path (e.g., "sandbox://...")
    std::string ResolvedPath;      // Resolved physical path
    std::string PreprocessedSource;// Source after #include resolution
    bool        IsLoaded = false;
};

// ============================================================
// ShaderManager — shader loading and #include preprocessing.
//
// Takes a FileSystem& reference (from Engine) for virtual-path
// resolution. LoadShader() reads a GLSL source file, recursively
// resolves #include directives, and returns a uint64_t handle.
//
// The #include preprocessing logic is extracted from OpenGLShader
// (ReadFileToString, CollectIncludeDirs, ResolveIncludes).
//
// Existing Shader classes (OpenGLShader, etc.) are NOT replaced
// yet — this is the centralized loading layer.
// ============================================================

class VKT_API ShaderManager
{
public:
    explicit ShaderManager(FileSystem& fileSystem);
    ~ShaderManager() = default;

    // Load a shader from a virtual path, preprocess #include directives,
    // and return a handle for later retrieval.
    uint64_t LoadShader(const std::string& virtualPath);

    // Retrieve preprocessed shader data by handle.
    // Returns nullptr if the handle is invalid.
    const ShaderData* GetShaderData(uint64_t handle) const;

    // Query whether a handle is valid.
    bool IsValidHandle(uint64_t handle) const;

    // Handle encoding (index + generation)
    static uint32_t GetIndex(uint64_t handle)      { return uint32_t(handle & 0xFFFFFFFF); }
    static uint16_t GetGeneration(uint64_t handle) { return uint16_t(handle >> 32); }
    static uint64_t MakeHandle(uint32_t index, uint16_t gen)
    {
        return (uint64_t(gen) << 32) | uint64_t(index);
    }

private:
    uint64_t AllocateHandle();

    // Preprocessing helpers (extracted from OpenGLShader)
    static std::string ReadFileToString(const std::filesystem::path& path);
    static void        CollectIncludeDirs(std::vector<std::filesystem::path>& outDirs,
                                          const std::filesystem::path& baseDir);
    static std::string ResolveIncludes(const std::string& source,
                                       const std::vector<std::filesystem::path>& includeDirs,
                                       std::unordered_set<std::string>& guardSet);

    FileSystem& m_FileSystem;

    // Shader storage: handle → ShaderData
    std::unordered_map<uint64_t, ShaderData> m_Shaders;

    // Handle management
    uint32_t          m_NextIndex = 0;     // Monotonically increasing index
    std::vector<uint32_t> m_FreeIndices;    // Recycled indices
};

} // namespace Vulkitten
