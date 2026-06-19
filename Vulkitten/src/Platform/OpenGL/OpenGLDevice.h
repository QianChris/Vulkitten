#pragma once

#include "Vulkitten/Renderer/Device.h"

namespace Vulkitten {

// ============================================================
// OpenGLDevice — OpenGL implementation of the Device interface.
//
// For OpenGL, the GL context IS the device — this is a thin
// wrapper that delegates to the existing OpenGL initialization
// and shutdown paths. In a future Vulkan backend, the Device
// implementation would own VkDevice / VkPhysicalDevice.
// ============================================================
class VKT_API OpenGLDevice : public Device
{
public:
    virtual void Init() override;
    virtual void Shutdown() override;
};

} // namespace Vulkitten
