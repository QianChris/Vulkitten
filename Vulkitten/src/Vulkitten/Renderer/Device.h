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
struct FrameContext;

class VKT_API IDevice
{
public:
    virtual ~IDevice() = default;

    virtual void Init() = 0;
    virtual void Shutdown() = 0;

    // Submit command buffers for the current frame and present.
    // For OpenGL: SwapBuffers. For Vulkan: vkQueueSubmit + Present.
    virtual void Submit(FrameContext& frameContext) = 0;

    static IDevice& Get();
};

} // namespace Vulkitten
