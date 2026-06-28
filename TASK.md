1. 四对合并：删除 Buffer.h/.cpp 和 OpenGLBuffer.h/.cpp — VertexBuffer/IndexBuffer/BufferLayout/BufferElement/ShaderDataType 全部移除。Buffer 创建统一走 IDevice::createBuffer(BufferDesc)，绑定走 ICommandBuffer::BindGeometry/BindUniformBuffer/BindStorageBuffer。顶点格式在 PipelineDesc::vertexLayout，不在 Buffer 上。删除后编译通过。
2. 四对合并：删除 VertexArray.h/.cpp 和 OpenGLVertexArray.h/.cpp — VertexArray(AddVertexBuffer/SetIndexBuffer/Bind/Unbind) 全部移除。VAO 由 GLCommandBuffer::BindGeometry 惰性创建（首次绑定时根据 PipelineDesc::vertexLayout + GeometryDesc::vertexBuffers 调用 glGenVertexArrays+glVertexAttribPointer，缓存复用）。删除后编译通过。
3. 四对合并：删除 Shader.h/.cpp 和 OpenGLShader.h/.cpp — Shader(Bind/Unbind/SetUniform*) 全部移除。Shader 创建走 IDevice::createShader(stage, bytecode)→ShaderHandle，绑定走 ICommandBuffer::BindPipeline，uniform 走 PushConstants+BindUniformBuffer+BindTexture。ShaderLibrary 从 Shader.h 中移出（在后续 Task 合并入 IGpuResourceManager）。删除后编译通过。
4. 四对合并：删除 Texture.h/.cpp 和 OpenGLTexture.h/.cpp — Texture2D(Bind/Unbind/GetWidth/GetHeight/SetData) 全部移除。Texture 创建走 IDevice::createTexture(TextureDesc)→TextureHandle，绑定走 ICommandBuffer::BindTexture(slot, handle, sampler)。删除后编译通过。
5. 清理死代码：删除 RendererAPI.h/.cpp 和 OpenGLRendererAPI.h/.cpp（已零外部调用者）。删除 RenderContext.h/.cpp 和 VkRenderContext.h/.cpp（已被 ICommandBuffer 替代）。删除 RenderCommand.h/.cpp（旧 Legacy::RenderCommand，非 RenderGraph）。更新 Vulkitten.h umbrella header 移除上述所有 include。
6. 合并 Renderer 基类到 OpenGLRenderer：将 Renderer.h/.cpp 的 m_Device/m_Resources/m_RenderGraph/m_ShaderLibrary/m_FrameContext 成员 + GetDevice/GetResourceManager/GetRenderGraph/GetShaderLibrary/OnWindowResize/BeginFrame/EndFrame 实现全部移入 OpenGLRenderer。VkRenderer 同步改为继承 OpenGLRenderer。Renderer.h/.cpp 删除。
7. 合并 ShaderLibrary 到 IGpuResourceManager：在 IGpuResourceManager 新增 LoadShader(name, virtualPath)→uint64_t 和 GetShaderByName(name)→Ref<Shader> 两个虚方法。OpenGLGpuResourceManager 内部持有 unordered_map<string, Ref<Shader>> 实现，VkGpuResourceManager 添加桩。Shader.h 中移除 ShaderLibrary 类（Shader 类本身已在 Task 3 删除）。更新所有引用（OpenGLRenderer::Init、GpuParticle、ExampleLayer2）改为通过 GetResourceManager() 访问。
8. 修复 FrameContext 传递链：OpenGLRenderer::BeginFrame 调用 IDevice::beginFrame() 产出真实 FrameContext（含 swapchainIndex）。RenderGraph::Execute 签名增加 FrameContext& 参数。PreparePass/SpriteRenderPass 的 onExecute 回调接收 FrameContext 引用，使用真实 ctx 而非 FrameContext{} 创建 ICommandBuffer。
9. 每帧复用单一 ICommandBuffer：OpenGLRenderer::BeginFrame 中调用 m_Device->createCommandBuffer(ctx) 创建一个 ICommandBuffer 并存储。Execute 中将此 cmd 传给 RenderGraph→所有 Pass 复用同一个 cmd（不再每 Pass new/delete）。EndFrame 在 m_Device->endFrame(ctx) 之前销毁 cmd。
10. SpriteRenderPass 完全迁移到 RHI：构造函数中所有资源通过 IDevice 创建——createBuffer(BufferDesc{size,Usage::Vertex})→QuadVB Handle、createBuffer(BufferDesc{size,Usage::Index})→QuadIB Handle、createShader(stage,SPIRV bytecode)→ShaderHandle、createPipeline(PipelineDesc{vertexLayout+TextureSlots})→PipelineHandle、createGeometry(GeometryDesc{QuadVB+QuadIB})→GeometryHandle、createSampler(SamplerDesc{})→SamplerHandle。Flush() 全部改为 ICommandBuffer 调用链：BindPipeline→BindGeometry(惰性 VAO)→PushConstants(VP matrix)→循环 BindTexture(slot,tex,sampler)→DrawIndexed。移除所有 Ref<VertexBuffer>/Ref<VertexArray>/Ref<Shader>/Ref<Texture2D> 旧成员。CPU 端 BatchVertex 数组改为 std::vector<BatchVertex>。
11. GpuParticlePass 使用 ICommandBuffer::DispatchCompute：4 个 compute pass(SimArg/Sim/Emit/RenderArg) 改为 cmd->DispatchCompute 替代直接 glDispatchCompute。SSBO 绑定通过 cmd->BindStorageBuffer。粒子渲染的 glDrawArraysIndirect 改为 cmd->Draw。着色器通过 IGpuResourceManager::LoadShader 加载。删除 GpuParticle.cpp 中所有原始 GL 调用。
12. 端到端验证 OpenGL 后端：运行 SandBox.exe（OpenGL 后端），确认清屏+3 Quad(Green/Red/Checkerboard)渲染正常+窗口 Resize 正常+GPU 粒子正常。确认无 RendererAPI/OpenGLRendererAPI/RenderContext/ShaderLibrary/Buffer/VertexArray/Shader/Texture2D 旧类残留。所有 3 个目标编译通过。
13. 更新 ARCHITECTURE.md 和 GOAL.md：重写渲染器分层架构图（仅保留 IRenderer→IDevice+ICommandBuffer+IGpuResourceManager+RenderGraph 四根支柱）。更新目录结构。GOAL.md 记录本轮清理完成状态。

# VulkittenRHI — 独立 RHI 库 (2026-06-24)

基于 RHI_Design_v1.2.md 在 `./VulkittenRHI/` 下创建独立的 RHI 静态库 + 样例。

## 设计评价 (v1.2 评分 8.5/10)
- 核心架构（Renderer → IRenderDevice + ICommandBuffer + RenderCommandList）清晰合理
- 调整点：Handle 保留 generation（防 ABA）、去除 ResourceManager 薄代理层、统一描述符到 ResourceDescs.hpp

## 实现任务 (8 Phases, 全部完成 ✅)

14. ✅ Phase 1 — 项目骨架：创建 VulkittenRHI/CMakeLists.txt（静态库，C++17，链接 GLFW/GLAD/OpenGL/Vulkan）、目录结构（include/rhi/Core/, src/gl/, src/vk/, samples/）
15. ✅ Phase 2 — Core 类型：Handle.hpp（带 generation 的强类型 Handle<Tag> + 9 种 Tag）、Format.hpp（Format 枚举 + 工具函数）、Types.hpp（所有枚举：ShaderStage, BufferUsage, TextureUsage, PipelineStage, AccessFlags, ImageLayout, LoadOp/StoreOp, CommandBufferLevel, QueueType + Extent/Viewport/ClearValue 等结构体）
16. ✅ Phase 3 — 资源描述符：ResourceDescs.hpp（BufferDesc, TextureDesc, VertexAttribute, RasterState, DepthStencilState, BlendState, TextureSlot, BufferSlot, PipelineDesc, GeometryDesc, SamplerDesc, AttachmentDesc, SubpassDesc, SubpassDependency, RenderPassDesc, FramebufferDesc）
17. ✅ Phase 4 — 抽象接口：ISurface.hpp, FrameContext.hpp, ICommandBuffer.hpp（完整命令录制接口 25 个虚方法）, IRenderDevice.hpp（帧生命周期 + 8 个 create* + 命令缓冲分配）, RenderCommandList.hpp（薄适配层，高层 DrawMesh + 链式 API）, Renderer.hpp（BackendType 枚举 + PIMPL 工厂类）
18. ✅ Phase 5 — OpenGL 后端：GLDevice（GL Context 管理 via GLFW + GLAD, Handle 池, beginFrame/endFrame/glfwSwapBuffers, createRenderPass/createFramebuffer/FBO, createCommandBuffer）, GLCommandBuffer（immediate 模式, 状态缓存, BeginRenderPass 执行 glClear, 懒 VAO 缓存）
19. ✅ Phase 6 — Vulkan 后端：VKSwapchain（VkSurface → VkSwapchainKHR + VkImage/VkImageView 数组 + VkRenderPass + per-image VkFramebuffer + per-frame Semaphore/Fence）, VKDevice（VkInstance → VkPhysicalDevice → VkDevice, beginFrame/acquireNextImage/endFrame/submit+present, CreateCommandBuffer/VkCommandPool 分配, OnResize 重建 swapchain）, VKCommandBuffer（真录制 vkBegin/EndCommandBuffer, BeginRenderPass/vkCmdBeginRenderPass with clear）
20. ✅ Phase 7 — Renderer 工厂：Renderer.cpp（PIMPL, Create() 工厂 switch BackendType 创建 GLDevice 或 VKDevice, BeginFrame/EndFrame 委托到 IDevice, GetDefaultRenderPass/GetDefaultFramebuffer 懒创建 swapchain 资源）, RenderCommandList.cpp（构造函数 + DrawMesh/DispatchCompute + 链式透传 Bind*/Draw*/Dispatch/Barrier）
21. ✅ Phase 8 — 样例 + 构建验证：samples/main.cpp（GLFWSurface 适配 ISurface, Renderer::Create(config), 主循环 BeginFrame→BeginRenderPass(clear red)→EndRenderPass→EndFrame, 仅改 BackendType 即可切换 GL/VK）, samples/CMakeLists.txt。OpenGL 后端编译通过，Vulkan 后端编译通过。
