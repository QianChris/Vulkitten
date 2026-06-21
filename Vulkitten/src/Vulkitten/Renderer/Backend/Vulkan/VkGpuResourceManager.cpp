#include "vktpch.h"
#include "VkGpuResourceManager.h"

#include "VulkanDevice.h"

namespace Vulkitten {

VkGpuResourceManager::VkGpuResourceManager(VulkanDevice& device)
    : m_Device(device)
{
}

VkGpuResourceManager::~VkGpuResourceManager()
{
}

uint32_t VkGpuResourceManager::AllocateSlot()
{
    if (!m_FreeIndices.empty())
    {
        uint32_t index = m_FreeIndices.back();
        m_FreeIndices.pop_back();
        auto& slot = m_Slots[index];
        slot.alive = true;
        slot.generation++;
        slot.type = GpuResourceSlot::Type::None;
        return index;
    }
    GpuResourceSlot slot;
    slot.alive = true;
    slot.generation = 0;
    m_Slots.push_back(slot);
    return uint32_t(m_Slots.size() - 1);
}

bool VkGpuResourceManager::ValidateHandle(uint32_t index, uint16_t generation) const
{
    if (index >= m_Slots.size()) return false;
    const auto& slot = m_Slots[index];
    return slot.alive && slot.generation == generation;
}

uint64_t VkGpuResourceManager::CreateTexture(const GpuTextureDesc& desc, const std::string& debugName)
{
    uint32_t index = AllocateSlot();
    auto& slot = m_Slots[index];
    slot.type = GpuResourceSlot::Type::Texture;
    slot.textureDesc = desc;
    slot.debugName = debugName;
    return MakeHandle(index, slot.generation);
}

uint64_t VkGpuResourceManager::CreateBuffer(const GpuBufferDesc& desc, const std::string& debugName)
{
    uint32_t index = AllocateSlot();
    auto& slot = m_Slots[index];
    slot.type = GpuResourceSlot::Type::Buffer;
    slot.bufferDesc = desc;
    slot.debugName = debugName;
    return MakeHandle(index, slot.generation);
}

uint64_t VkGpuResourceManager::CreateShader(const std::string& name, const std::string& /*source*/)
{
    uint32_t index = AllocateSlot();
    auto& slot = m_Slots[index];
    slot.debugName = name;
    return MakeHandle(index, slot.generation);
}

uint64_t VkGpuResourceManager::CreatePipeline(const void* /*pipelineDesc*/)
{
    uint32_t index = AllocateSlot();
    auto& slot = m_Slots[index];
    slot.debugName = "VkPipeline";
    return MakeHandle(index, slot.generation);
}

uint64_t VkGpuResourceManager::CreateGeometry(const void* /*geometryDesc*/)
{
    uint32_t index = AllocateSlot();
    auto& slot = m_Slots[index];
    slot.debugName = "VkGeometry";
    return MakeHandle(index, slot.generation);
}

GpuResourceSlot* VkGpuResourceManager::GetTexture(uint64_t handle)
{
    uint32_t index = GetIndex(handle);
    uint16_t gen   = GetGeneration(handle);
    if (!ValidateHandle(index, gen)) return nullptr;
    m_Slots[index].lastUsedFrame = m_CurrentFrame;
    return &m_Slots[index];
}

GpuResourceSlot* VkGpuResourceManager::GetBuffer(uint64_t handle)
{
    uint32_t index = GetIndex(handle);
    uint16_t gen   = GetGeneration(handle);
    if (!ValidateHandle(index, gen)) return nullptr;
    m_Slots[index].lastUsedFrame = m_CurrentFrame;
    return &m_Slots[index];
}

GpuResourceSlot* VkGpuResourceManager::GetSlot(uint32_t index)
{
    if (index >= m_Slots.size()) return nullptr;
    return &m_Slots[index];
}

void VkGpuResourceManager::TrackExternalRef(uint64_t handle, const std::weak_ptr<void>& tracker)
{
    uint32_t index = GetIndex(handle);
    if (ValidateHandle(index, GetGeneration(handle)))
        m_ExternalTrackers[index] = tracker;
}

void VkGpuResourceManager::SetGpuHandle(uint64_t handle, uint64_t gpuHandle)
{
    uint32_t index = GetIndex(handle);
    if (ValidateHandle(index, GetGeneration(handle)))
        m_Slots[index].gpuHandle = gpuHandle;
}

void VkGpuResourceManager::DestroyResource(uint64_t handle)
{
    uint32_t index = GetIndex(handle);
    uint16_t gen   = GetGeneration(handle);
    if (!ValidateHandle(index, gen)) return;
    m_Slots[index].alive = false;
    m_Slots[index].type = GpuResourceSlot::Type::None;
    m_ExternalTrackers.erase(index);
    m_FreeIndices.push_back(index);
}

void VkGpuResourceManager::TickFrame()
{
    m_CurrentFrame++;
}

void VkGpuResourceManager::Gc(uint32_t maxFramesInFlight)
{
    for (uint32_t i = 0; i < m_Slots.size(); i++)
    {
        auto& slot = m_Slots[i];
        if (!slot.alive) continue;
        if (m_CurrentFrame - slot.lastUsedFrame > maxFramesInFlight)
        {
            auto it = m_ExternalTrackers.find(i);
            if (it != m_ExternalTrackers.end() && !it->second.expired())
                continue;
            DestroyResource(MakeHandle(i, slot.generation));
        }
    }
}

size_t VkGpuResourceManager::GetResourceCount() const
{
    return m_Slots.size() - m_FreeIndices.size();
}

uint64_t VkGpuResourceManager::LoadShader(const std::string& /*virtualPath*/)
{
    // Stub: Vulkan loads .spv directly, no GLSL preprocessing needed
    return 0;
}

const ShaderData* VkGpuResourceManager::GetShaderData(uint64_t /*handle*/) const
{
    return nullptr;
}

} // namespace Vulkitten
