#pragma once

#include "Vulkitten/Core/Core.h"
#include "Vulkitten/Renderer/Device.h"

namespace Vulkitten {

class VulkanInstance;

// ============================================================
// VulkanDevice — Vulkan implementation of IDevice.
//
// Handles physical device enumeration (preferring discrete GPU),
// logical device creation, and queue family selection (graphics,
// present, transfer). Registered via ClassFactory::RegisterInterface.
// ============================================================
class VulkanDevice : public IDevice
{
public:
    explicit VulkanDevice(VulkanInstance& instance);
    ~VulkanDevice();

    void Init() override;
    void Shutdown() override;

    // ---- Queue Family Access ----
    uint32_t GetGraphicsQueueFamily() const { return m_GraphicsQueueFamily; }
    uint32_t GetPresentQueueFamily()  const { return m_PresentQueueFamily; }
    uint32_t GetTransferQueueFamily() const { return m_TransferQueueFamily; }

    void* GetNativeDevice()        const { return m_NativeDevice; }
    void* GetNativePhysicalDevice() const { return m_PhysicalDevice; }

private:
    VulkanInstance& m_Instance;
    void* m_PhysicalDevice = nullptr;  // VkPhysicalDevice
    void* m_NativeDevice = nullptr;    // VkDevice (Vulkan typedef, not class name)
    uint32_t m_GraphicsQueueFamily = 0;
    uint32_t m_PresentQueueFamily = 0;
    uint32_t m_TransferQueueFamily = 0;
};

} // namespace Vulkitten
