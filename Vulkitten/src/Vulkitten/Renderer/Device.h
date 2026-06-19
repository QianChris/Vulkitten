#pragma once

#include "Vulkitten/Core/Core.h"

namespace Vulkitten {

// ============================================================
// Device — abstract GPU device interface.
//
// Represents a logical GPU device. For OpenGL this is a thin
// wrapper (the GL context IS the device). For Vulkan this would
// own VkDevice + VkPhysicalDevice + queue families.
//
// Created and registered via ClassFactory::GetInterface<Device>()
// with the concrete implementation selected by the rendering
// backend (OpenGL / Vulkan).
// ============================================================
class VKT_API Device
{
public:
    virtual ~Device() = default;

    // Initialize the GPU device.
    virtual void Init() = 0;

    // Shutdown and release GPU resources.
    virtual void Shutdown() = 0;

    // Convenience accessor — retrieves the registered Device
    // implementation via ClassFactory::GetInterface<Device>().
    static Device& Get();
};

} // namespace Vulkitten
