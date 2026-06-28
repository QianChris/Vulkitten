#pragma once

#include "Vulkitten/Core/Core.h"

namespace Vulkitten {

// ============================================================
// VulkanInstance - Vulkan instance wrapper with debug messenger.
//
// Encapsulates VkInstance creation, validation layer setup,
// and VK_EXT_debug_utils debug messenger. Created once at
// engine startup and shared by all Vulkan backend components.
// ============================================================
class VulkanInstance
{
public:
    VulkanInstance();
    ~VulkanInstance();

    // Initialize: create VkInstance + debug messenger.
    // enableValidationLayers: if true, enables VK_LAYER_KHRONOS_validation.
    void Init(bool enableValidationLayers = true);

    // Shutdown: destroy debug messenger + VkInstance.
    void Shutdown();

    // Returns the native VkInstance handle (nullptr if not initialized).
    void* GetNativeInstance() const { return m_Instance; }

    // Returns true if the Vulkan instance has been created successfully.
    bool IsValid() const { return m_Instance != nullptr; }

private:
    void* m_Instance = nullptr;       // VkInstance
    void* m_DebugMessenger = nullptr; // VkDebugUtilsMessengerEXT
};

} // namespace Vulkitten
