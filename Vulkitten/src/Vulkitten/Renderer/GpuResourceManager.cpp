#include "vktpch.h"
#include "GpuResourceManager.h"

#include "Vulkitten/Core/FileSystem.h"
#include "Vulkitten/Core/Engine.h"
#include "Vulkitten/Perf/Instrumentor.h"

#include <fstream>
#include <sstream>

namespace Vulkitten {

GpuResourceManager::GpuResourceManager(FileSystem& fileSystem)
    : m_FileSystem(fileSystem)
{
}

// ============================================================
// Slot Allocation (free-list with generation bump)
// ============================================================

uint32_t GpuResourceManager::AllocateSlot()
{
    // Reuse a freed slot if available
    if (!m_FreeIndices.empty())
    {
        uint32_t index = m_FreeIndices.back();
        m_FreeIndices.pop_back();

        auto& slot = m_Slots[index];
        slot.alive = true;
        slot.generation++;          // Invalidate old handles pointing here
        slot.type = GpuResourceSlot::Type::None;
        slot.deferred = true;
        slot.gpuHandle = 0;
        return index;
    }

    // Allocate a new slot
    GpuResourceSlot slot;
    slot.alive = true;
    slot.generation = 0;
    m_Slots.push_back(slot);
    return uint32_t(m_Slots.size() - 1);
}

// ============================================================
// Handle Validation
// ============================================================

bool GpuResourceManager::ValidateHandle(uint32_t index, uint16_t generation) const
{
    if (index >= m_Slots.size())
        return false;

    const auto& slot = m_Slots[index];
    return slot.alive && slot.generation == generation;
}

// ============================================================
// Resource Creation
// ============================================================

uint64_t GpuResourceManager::CreateTexture(const GpuTextureDesc& desc, const std::string& debugName)
{
    uint32_t index = AllocateSlot();

    auto& slot = m_Slots[index];
    slot.type = GpuResourceSlot::Type::Texture;
    slot.textureDesc = desc;
    slot.deferred = true;
    slot.gpuHandle = 0;
    slot.debugName = debugName;

    return MakeHandle(index, slot.generation);
}

uint64_t GpuResourceManager::CreateBuffer(const GpuBufferDesc& desc, const std::string& debugName)
{
    uint32_t index = AllocateSlot();

    auto& slot = m_Slots[index];
    slot.type = GpuResourceSlot::Type::Buffer;
    slot.bufferDesc = desc;
    slot.deferred = true;
    slot.gpuHandle = 0;
    slot.debugName = debugName;

    return MakeHandle(index, slot.generation);
}

// ============================================================
// Stub implementations for shader/pipeline/geometry (Task 3)
// These are placeholders until the Vulkan backend provides real allocators.
// ============================================================

uint64_t GpuResourceManager::CreateShader(const std::string& name, const std::string& source)
{
    uint32_t index = AllocateSlot();
    auto& slot = m_Slots[index];
    slot.type = GpuResourceSlot::Type::None; // Future: Type::Shader
    slot.deferred = true;
    slot.gpuHandle = 0;
    slot.debugName = name;
    return MakeHandle(index, slot.generation);
}

uint64_t GpuResourceManager::CreatePipeline(const void* pipelineDesc)
{
    uint32_t index = AllocateSlot();
    auto& slot = m_Slots[index];
    slot.type = GpuResourceSlot::Type::None; // Future: Type::Pipeline
    slot.deferred = true;
    slot.gpuHandle = 0;
    slot.debugName = "Pipeline";
    return MakeHandle(index, slot.generation);
}

uint64_t GpuResourceManager::CreateGeometry(const void* geometryDesc)
{
    uint32_t index = AllocateSlot();
    auto& slot = m_Slots[index];
    slot.type = GpuResourceSlot::Type::None; // Future: Type::Geometry
    slot.deferred = true;
    slot.gpuHandle = 0;
    slot.debugName = "Geometry";
    return MakeHandle(index, slot.generation);
}

// ============================================================
// Resource Lookup (triggers deferred allocation)
// ============================================================

GpuResourceSlot* GpuResourceManager::GetTexture(uint64_t handle)
{
    uint32_t  index = GetIndex(handle);
    uint16_t  gen   = GetGeneration(handle);

    if (!ValidateHandle(index, gen))
        return nullptr;

    auto& slot = m_Slots[index];
    if (slot.type != GpuResourceSlot::Type::Texture)
        return nullptr;

    if (slot.deferred)
    {
        // Deferred GPU allocation — placeholder for now.
        // In the future: glGenTextures / vkCreateImage
        slot.deferred = false;
        VKT_CORE_INFO("GpuResourceManager: deferred texture creation '{0}' ({1}x{2})",
                      slot.debugName, slot.textureDesc.Width, slot.textureDesc.Height);
    }

    slot.lastUsedFrame = m_CurrentFrame;
    return &slot;
}

GpuResourceSlot* GpuResourceManager::GetBuffer(uint64_t handle)
{
    uint32_t  index = GetIndex(handle);
    uint16_t  gen   = GetGeneration(handle);

    if (!ValidateHandle(index, gen))
        return nullptr;

    auto& slot = m_Slots[index];
    if (slot.type != GpuResourceSlot::Type::Buffer)
        return nullptr;

    if (slot.deferred)
    {
        // Deferred GPU allocation — placeholder for now.
        slot.deferred = false;
        VKT_CORE_INFO("GpuResourceManager: deferred buffer creation '{0}' ({1} bytes)",
                      slot.debugName, slot.bufferDesc.Size);
    }

    slot.lastUsedFrame = m_CurrentFrame;
    return &slot;
}

// ============================================================
// Low-level Slot Access
// ============================================================

GpuResourceSlot* GpuResourceManager::GetSlot(uint32_t index)
{
    if (index >= m_Slots.size())
        return nullptr;
    return &m_Slots[index];
}

// ============================================================
// External Reference Tracking
// ============================================================

void GpuResourceManager::TrackExternalRef(uint64_t handle, const std::weak_ptr<void>& tracker)
{
    uint32_t index = GetIndex(handle);
    uint16_t gen   = GetGeneration(handle);

    if (!ValidateHandle(index, gen))
        return;

    m_ExternalTrackers[index] = tracker;
}

void GpuResourceManager::SetGpuHandle(uint64_t handle, uint64_t gpuHandle)
{
    uint32_t index = GetIndex(handle);
    uint16_t gen   = GetGeneration(handle);

    if (!ValidateHandle(index, gen))
        return;

    m_Slots[index].gpuHandle = gpuHandle;
    m_Slots[index].deferred = false;
}

// ============================================================
// Resource Destruction
// ============================================================

void GpuResourceManager::DestroyResource(uint64_t handle)
{
    uint32_t  index = GetIndex(handle);
    uint16_t  gen   = GetGeneration(handle);

    if (!ValidateHandle(index, gen))
        return;

    auto& slot = m_Slots[index];
    slot.alive = false;
    slot.type  = GpuResourceSlot::Type::None;
    // Future: glDeleteTextures / vkDestroyImage using slot.gpuHandle

    m_ExternalTrackers.erase(index);
    m_FreeIndices.push_back(index);
}

// ============================================================
// Frame Management
// ============================================================

void GpuResourceManager::TickFrame()
{
    m_CurrentFrame++;
}

void GpuResourceManager::Gc(uint32_t maxFramesInFlight)
{
    // Collect indices of resources eligible for garbage collection.
    // A resource is eligible when:
    //   1. It is alive.
    //   2. It hasn't been accessed for `maxFramesInFlight` frames.
    //   3. No external Ref (Texture2D/Buffer shared_ptr) still holds it.
    std::vector<uint64_t> toDestroy;

    for (uint32_t i = 0; i < m_Slots.size(); i++)
    {
        auto& slot = m_Slots[i];
        if (!slot.alive)
            continue;

        uint32_t unusedFrames = m_CurrentFrame - slot.lastUsedFrame;
        if (unusedFrames > maxFramesInFlight)
        {
            // Check if the resource is still externally held via Ref<>.
            auto it = m_ExternalTrackers.find(i);
            if (it != m_ExternalTrackers.end() && !it->second.expired())
            {
                // Still has external owners — skip this resource.
                continue;
            }
            toDestroy.push_back(MakeHandle(i, slot.generation));
        }
    }

    for (uint64_t handle : toDestroy)
    {
        uint32_t index = GetIndex(handle);
        VKT_CORE_INFO("GpuResourceManager::Gc — destroying unused resource '{0}' "
                      "(unused for {1} frames)",
                      m_Slots[index].debugName,
                      m_CurrentFrame - m_Slots[index].lastUsedFrame);
        DestroyResource(handle);
    }

    if (!toDestroy.empty())
        VKT_CORE_INFO("GpuResourceManager::Gc — cleaned up {0} resources", toDestroy.size());
}

// ============================================================
// Shader Loading (formerly ShaderManager)
// ============================================================

uint64_t GpuResourceManager::AllocateShaderHandle()
{
    uint32_t index;
    if (!m_FreeShaderIndices.empty())
    {
        index = m_FreeShaderIndices.back();
        m_FreeShaderIndices.pop_back();
    }
    else
    {
        index = m_NextShaderIndex++;
    }
    return (uint64_t(index));
}

uint64_t GpuResourceManager::LoadShader(const std::string& virtualPath)
{
    VKT_PROFILE_RENDER_FUNCTION();

    std::string resolvedPath = m_FileSystem.Resolve(virtualPath);

    std::string source = ReadFileToString(resolvedPath);
    if (source.empty())
    {
        VKT_CORE_ERROR("GpuResourceManager::LoadShader — failed to read: {0}", virtualPath);
        return UINT64_MAX;
    }

    std::filesystem::path baseDir = std::filesystem::path(resolvedPath).parent_path();
    std::vector<std::filesystem::path> includeDirs;
    CollectIncludeDirs(includeDirs, baseDir);

    std::unordered_set<std::string> guardSet;
    std::string preprocessed = ResolveIncludes(source, includeDirs, guardSet);

    uint64_t handle = AllocateShaderHandle();

    ShaderData data;
    data.VirtualPath        = virtualPath;
    data.ResolvedPath       = resolvedPath;
    data.PreprocessedSource = preprocessed;
    data.IsLoaded           = true;

    m_Shaders[handle] = std::move(data);

    VKT_CORE_INFO("GpuResourceManager::LoadShader — loaded '{0}'", virtualPath);
    return handle;
}

const ShaderData* GpuResourceManager::GetShaderData(uint64_t handle) const
{
    auto it = m_Shaders.find(handle);
    return (it != m_Shaders.end()) ? &it->second : nullptr;
}

// ============================================================
// Preprocessing Helpers (extracted from OpenGLShader)
// ============================================================

std::string GpuResourceManager::ReadFileToString(const std::filesystem::path& path)
{
    std::ifstream in(path, std::ios::in | std::ios::binary);
    if (!in.is_open())
        return {};
    std::stringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

void GpuResourceManager::CollectIncludeDirs(
    std::vector<std::filesystem::path>& outDirs,
    const std::filesystem::path& baseDir)
{
    outDirs.push_back(baseDir);
    auto dir = baseDir;
    for (int i = 0; i < 10; ++i)
    {
        auto candidate = dir / "Vulkitten";
        if (std::filesystem::exists(candidate / "src"))
        {
            outDirs.push_back(candidate / "src");
            return;
        }
        auto parent = dir.parent_path();
        if (parent == dir) break;
        dir = parent;
    }
}

std::string GpuResourceManager::ResolveIncludes(
    const std::string& source,
    const std::vector<std::filesystem::path>& includeDirs,
    std::unordered_set<std::string>& guardSet)
{
    std::string result;
    std::istringstream stream(source);
    std::string line;

    while (std::getline(stream, line))
    {
        std::string trimmed = line;
        {
            auto pos = trimmed.find_first_not_of(" \t\r");
            if (pos != std::string::npos)
                trimmed = trimmed.substr(pos);
            else
                trimmed.clear();
        }

        if (trimmed.rfind("#extension", 0) == 0 &&
            trimmed.find("GL_GOOGLE_include_directive") != std::string::npos)
        {
            result += "// " + line + "\n";
            continue;
        }

        if (trimmed.rfind("#include", 0) == 0)
        {
            auto q1 = trimmed.find('"');
            auto q2 = trimmed.find('"', q1 + 1);
            if (q1 != std::string::npos && q2 != std::string::npos)
            {
                std::string incPath = trimmed.substr(q1 + 1, q2 - q1 - 1);
                bool found = false;
                for (const auto& d : includeDirs)
                {
                    auto full = d / incPath;
                    auto normal = std::filesystem::weakly_canonical(full);
                    if (std::filesystem::exists(normal))
                    {
                        std::string key = normal.string();
                        if (guardSet.count(key))
                        {
                            result += "// already included: " + incPath + "\n";
                        }
                        else
                        {
                            guardSet.insert(key);
                            std::string incSrc = ReadFileToString(normal);
                            if (!incSrc.empty())
                                result += ResolveIncludes(incSrc, includeDirs, guardSet);
                        }
                        found = true;
                        break;
                    }
                }
                if (!found)
                    result += line + "\n";
            }
            else
            {
                result += line + "\n";
            }
        }
        else
        {
            result += line + "\n";
        }
    }
    return result;
}

} // namespace Vulkitten
