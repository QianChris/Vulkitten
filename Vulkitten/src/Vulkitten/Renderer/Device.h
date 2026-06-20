#pragma once

#include "Vulkitten/Core/Core.h"

namespace Vulkitten {

// ============================================================
// IDevice — abstract GPU device interface.
//
// Represents a logical GPU device. For OpenGL this is a thin
// wrapper (the GL context IS the device). For Vulkan this would
// own VkDevice + VkPhysicalDevice + queue families.
//
// Created and registered via ClassFactory::RegisterInterface<IDevice>()
// with the concrete implementation selected by the rendering
// backend (OpenGL / Vulkan).
// ============================================================
class VKT_API IDevice
{
public:
    virtual ~IDevice() = default;

    // Initialize the GPU device (physical device selection,
    // logical device creation, queue family queries).
    virtual void Init() = 0;

    // Shutdown and release GPU resources.
    virtual void Shutdown() = 0;

    // Convenience accessor — retrieves the registered IDevice
    // implementation via ClassFactory::GetInterface<IDevice>().
    static IDevice& Get();
};

} // namespace Vulkitten
