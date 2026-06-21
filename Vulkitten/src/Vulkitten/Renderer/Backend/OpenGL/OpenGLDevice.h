#pragma once

#include "Vulkitten/Renderer/Device.h"
#include "Vulkitten/Renderer/FrameContext.h"

namespace Vulkitten {

// ============================================================
// OpenGLDevice — OpenGL implementation of the Device interface.
//
// For OpenGL, the GL context IS the device — this is a thin
// wrapper that delegates to the existing OpenGL initialization
// and shutdown paths. In a future Vulkan backend, the Device
// implementation would own VkDevice / VkPhysicalDevice.
// ============================================================
class VKT_API OpenGLDevice : public IDevice
{
public:
    explicit OpenGLDevice(void* nativeWindow = nullptr);
    void Init() override;
    void Shutdown() override;
    void Submit(FrameContext& frameContext) override;

private:
    void* m_NativeWindow = nullptr; // GLFWwindow*
};

} // namespace Vulkitten
