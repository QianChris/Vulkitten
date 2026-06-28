# VulkittenRHI — Render Hardware Interface

跨平台 GPU 抽象层，同时支持 **OpenGL 4.6** 和 **Vulkan 1.3**。

## 快速开始

```cpp
#include <rhi/Renderer.hpp>

// 1. 创建窗口 surface
GLFWSurface surface(window);

// 2. 创建 Renderer
rhi::RendererConfig config;
config.Backend = rhi::BackendType::OpenGL;  // 或 Vulkan
config.Surface = &surface;
auto renderer = rhi::Renderer::Create(config);

// 3. 主循环
while (running) {
    renderer->BeginFrame();
    rhi::ICommandBuffer& cmd = renderer->GetCommandBuffer();

    rhi::ClearValue cv;
    cv.Color.RGBA = {0.1f, 0.1f, 0.1f, 1.0f};
    cmd.BeginRenderPass(rp, fb, area, &cv, 1);
    cmd.BindPipeline(pipeline);
    cmd.BindGeometry(geometry);
    cmd.Draw(3);
    cmd.EndRenderPass();

    renderer->EndFrame();
}
```

## 构建

从项目根目录：

```bash
cd VulkittenEngine
cmake -B build -S .
cmake --build build --config Debug
```

需要：
- **GL 后端**: OpenGL 4.6 驱动
- **VK 后端**: [Vulkan SDK 1.3+](https://vulkan.lunarg.com/)

## 切换到不同后端

```cpp
// 一行切换
rhi::BackendType backend = rhi::BackendType::OpenGL;   // GL 4.6
rhi::BackendType backend = rhi::BackendType::Vulkan;    // Vulkan 1.3
```

## 运行测试

```bash
# 无 GPU 单元测试（接口契约、类型、Format）
./bin/Debug-x64/VulkittenRHIUnitTests.exe

# OpenGL 后端测试（需要 GPU）
./bin/Debug-x64/VulkittenRHIGLTests.exe

# Vulkan 后端测试（需要 GPU + Vulkan SDK）
./bin/Debug-x64/VulkittenRHIVKTests.exe
```
