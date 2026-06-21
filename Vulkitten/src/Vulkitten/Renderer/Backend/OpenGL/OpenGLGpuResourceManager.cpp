#include "vktpch.h"
#include "OpenGLGpuResourceManager.h"

#include "Vulkitten/Core/FileSystem.h"
#include "Vulkitten/Core/Engine.h"
#include "Vulkitten/Perf/Instrumentor.h"

#include <fstream>
#include <sstream>
#include <filesystem>
#include <unordered_set>

namespace Vulkitten {

OpenGLGpuResourceManager::OpenGLGpuResourceManager(FileSystem& fileSystem)
    : m_FileSystem(fileSystem)
{
}

// ============================================================
// Slot Allocation
// ============================================================

uint32_t OpenGLGpuResourceManager::AllocateSlot()
{
    if (!m_FreeIndices.empty())
    {
        uint32_t index = m_FreeIndices.back();
        m_FreeIndices.pop_back();
        auto& slot = m_Slots[index];
        slot.alive = true;
        slot.generation++;
        slot.type = GpuResourceSlot::Type::None;
        slot.deferred = true;
        slot.gpuHandle = 0;
        return index;
    }
    GpuResourceSlot slot;
    slot.alive = true;
    slot.generation = 0;
    m_Slots.push_back(slot);
    return uint32_t(m_Slots.size() - 1);
}

bool OpenGLGpuResourceManager::ValidateHandle(uint32_t index, uint16_t generation) const
{
    if (index >= m_Slots.size()) return false;
    const auto& slot = m_Slots[index];
    return slot.alive && slot.generation == generation;
}

uint64_t OpenGLGpuResourceManager::CreateTexture(const GpuTextureDesc& desc, const std::string& debugName)
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

uint64_t OpenGLGpuResourceManager::CreateBuffer(const GpuBufferDesc& desc, const std::string& debugName)
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

uint64_t OpenGLGpuResourceManager::CreateShader(const std::string& name, const std::string&)
{
    uint32_t index = AllocateSlot();
    auto& slot = m_Slots[index];
    slot.debugName = name;
    return MakeHandle(index, slot.generation);
}

uint64_t OpenGLGpuResourceManager::CreatePipeline(const void*)
{
    uint32_t index = AllocateSlot();
    auto& slot = m_Slots[index];
    slot.debugName = "Pipeline";
    return MakeHandle(index, slot.generation);
}

uint64_t OpenGLGpuResourceManager::CreateGeometry(const void*)
{
    uint32_t index = AllocateSlot();
    auto& slot = m_Slots[index];
    slot.debugName = "Geometry";
    return MakeHandle(index, slot.generation);
}

GpuResourceSlot* OpenGLGpuResourceManager::GetTexture(uint64_t handle)
{
    uint32_t index = GetIndex(handle);
    uint16_t gen   = GetGeneration(handle);
    if (!ValidateHandle(index, gen)) return nullptr;
    auto& slot = m_Slots[index];
    slot.lastUsedFrame = m_CurrentFrame;
    return &slot;
}

GpuResourceSlot* OpenGLGpuResourceManager::GetBuffer(uint64_t handle)
{
    uint32_t index = GetIndex(handle);
    uint16_t gen   = GetGeneration(handle);
    if (!ValidateHandle(index, gen)) return nullptr;
    auto& slot = m_Slots[index];
    slot.lastUsedFrame = m_CurrentFrame;
    return &slot;
}

GpuResourceSlot* OpenGLGpuResourceManager::GetSlot(uint32_t index)
{
    if (index >= m_Slots.size()) return nullptr;
    return &m_Slots[index];
}

void OpenGLGpuResourceManager::TrackExternalRef(uint64_t handle, const std::weak_ptr<void>& tracker)
{
    uint32_t index = GetIndex(handle);
    uint16_t gen   = GetGeneration(handle);
    if (!ValidateHandle(index, gen)) return;
    m_ExternalTrackers[index] = tracker;
}

void OpenGLGpuResourceManager::SetGpuHandle(uint64_t handle, uint64_t gpuHandle)
{
    uint32_t index = GetIndex(handle);
    uint16_t gen   = GetGeneration(handle);
    if (!ValidateHandle(index, gen)) return;
    m_Slots[index].gpuHandle = gpuHandle;
    m_Slots[index].deferred = false;
}

void OpenGLGpuResourceManager::DestroyResource(uint64_t handle)
{
    uint32_t index = GetIndex(handle);
    uint16_t gen   = GetGeneration(handle);
    if (!ValidateHandle(index, gen)) return;
    auto& slot = m_Slots[index];
    slot.alive = false;
    slot.type  = GpuResourceSlot::Type::None;
    m_ExternalTrackers.erase(index);
    m_FreeIndices.push_back(index);
}

void OpenGLGpuResourceManager::TickFrame()
{
    m_CurrentFrame++;
}

void OpenGLGpuResourceManager::Gc(uint32_t maxFramesInFlight)
{
    std::vector<uint64_t> toDestroy;
    for (uint32_t i = 0; i < m_Slots.size(); i++)
    {
        auto& slot = m_Slots[i];
        if (!slot.alive) continue;
        uint32_t unusedFrames = m_CurrentFrame - slot.lastUsedFrame;
        if (unusedFrames > maxFramesInFlight)
        {
            auto it = m_ExternalTrackers.find(i);
            if (it != m_ExternalTrackers.end() && !it->second.expired())
                continue;
            toDestroy.push_back(MakeHandle(i, slot.generation));
        }
    }
    for (uint64_t h : toDestroy)
        DestroyResource(h);
    if (!toDestroy.empty())
        VKT_CORE_INFO("OpenGLGpuResourceManager::Gc — cleaned up {0} resources", toDestroy.size());
}

// ---- Shader Loading ----

uint64_t OpenGLGpuResourceManager::AllocateShaderHandle()
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
    return uint64_t(index);
}

uint64_t OpenGLGpuResourceManager::LoadShader(const std::string& virtualPath)
{
    VKT_PROFILE_RENDER_FUNCTION();
    std::string resolvedPath = m_FileSystem.Resolve(virtualPath);
    std::string source = ReadFileToString(resolvedPath);
    if (source.empty())
    {
        VKT_CORE_ERROR("OpenGLGpuResourceManager::LoadShader — failed: {0}", virtualPath);
        return UINT64_MAX;
    }
    std::filesystem::path baseDir(resolvedPath);
    std::vector<std::filesystem::path> includeDirs;
    CollectIncludeDirs(includeDirs, baseDir.parent_path());
    std::unordered_set<std::string> guardSet;
    std::string preprocessed = ResolveIncludes(source, includeDirs, guardSet);
    uint64_t handle = AllocateShaderHandle();
    ShaderData data;
    data.VirtualPath = virtualPath;
    data.ResolvedPath = resolvedPath;
    data.PreprocessedSource = preprocessed;
    data.IsLoaded = true;
    m_Shaders[handle] = std::move(data);
    return handle;
}

const ShaderData* OpenGLGpuResourceManager::GetShaderData(uint64_t handle) const
{
    auto it = m_Shaders.find(handle);
    return (it != m_Shaders.end()) ? &it->second : nullptr;
}

std::string OpenGLGpuResourceManager::ReadFileToString(const std::filesystem::path& path)
{
    std::ifstream in(path, std::ios::in | std::ios::binary);
    if (!in.is_open()) return {};
    std::stringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

void OpenGLGpuResourceManager::CollectIncludeDirs(
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

std::string OpenGLGpuResourceManager::ResolveIncludes(
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
            if (pos != std::string::npos) trimmed = trimmed.substr(pos);
            else trimmed.clear();
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
                            result += "// already included: " + incPath + "\n";
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
                if (!found) result += line + "\n";
            }
            else result += line + "\n";
        }
        else result += line + "\n";
    }
    return result;
}

} // namespace Vulkitten
