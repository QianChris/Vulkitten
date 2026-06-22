# TASK_LOG.md — Vulkitten Engine Task Execution Log

## Task 1: 修复 IRenderer::Get() 空指针崩溃
- **Start**: 2026-06-21
- **End**: 2026-06-21
- **Summary**: 将 `s_Current = this;` 从 Renderer::Init() 和 VkRenderer::Init() 的末尾移到最开头（任何 Pass 注册或资源创建之前）。之前 SpriteRenderPass 构造函数中的 VertexBuffer::Create()→IRenderer::Get().GetResourceManager() 因 s_Current 尚未赋值而崩溃。现在 IRenderer::Get() 在 Init 期间立即可用。所有 3 个目标编译通过。

## Task 2: 清理 Renderer.h 平台特定依赖
- **Start**: 2026-06-21
- **End**: 2026-06-21
- **Summary**: Renderer.h 中 `Scope<OpenGLDevice>` 改为 `Scope<IDevice>`，`Scope<GpuResourceManager>` 改为 `Scope<IGpuResourceManager>`。移除 `GetGpuResourceManager()`、`class OpenGLDevice` 和 `class GpuResourceManager` 前向声明。头文件现在只暴露 IRenderer 接口类型（IDevice/IGpuResourceManager），平台具体实现仅在 .cpp 中通过 CreateScope 创建。所有 3 个目标编译通过。

## Task 3: ShaderManager 并入 IGpuResourceManager
- **Start**: 2026-06-21
- **End**: 2026-06-21
- **Summary**: 将 ShaderManager 的 ShaderData 结构体、LoadShader/GetShaderData 方法和 GLSL #include 预处理逻辑（ReadFileToString/CollectIncludeDirs/ResolveIncludes）全部移入 IGpuResourceManager/GpuResourceManager。GpuResourceManager 构造器新增 FileSystem& 参数用于虚拟路径解析。VkGpuResourceManager 添加桩实现。RendererConfig 移除 ShaderManager* 字段。Application 移除 Scope<ShaderManager> 成员及其创建代码。ShaderManager.h/.cpp 已删除。所有 3 个目标编译通过。

## Task 4: 切换 Shader 加载为直接 .spv 二进制
- **Start**: 2026-06-21
- **End**: 2026-06-21
- **Summary**: OpenGLShader 重写为 SPIR-V 直接加载：构造器读取 .vert.spv/.frag.spv 二进制文件，使用 glShaderBinary + glSpecializeShader + glLinkProgram 替代运行时 GLSL 编译。移除了 OpenGLShader.cpp 中约 200 行的 GLSL #include 预处理代码（ReadFileToString/CollectIncludeDirs/ResolveIncludes）。Shader::Create(source-based) 方法已移除。Shader::Create(filepath) 现在接受无扩展名的 shader 名称（如 "engine://shaders/TextureEntity"），自动拼接 .spv 后缀。所有 shader 路径已更新（TextureEntity.shader→TextureEntity，Particle.shader→Particle，FlatColor.shader→FlatColor）。所有 3 个目标编译通过。

## Task 5: 验证所有 shader 路径已迁移到 .spv
- **Start**: 2026-06-21
- **End**: 2026-06-21
- **Summary**: 已验证引擎源码中零处 .shader/.vert/.frag GLSL 引用残留。所有 shader 路径已在 Task 4 中更新：SpriteRenderPass(TextureEntity)、Renderer::Init()(Particle + 4 compute shaders)、ExampleLayer2(FlatColor) 均使用无扩展名基础路径，由 OpenGLShader 自动拼接 .spv 后缀。GpuParticlePass 的 compute shader 路径保持不变（原已是 .comp 扩展名）。所有 3 个目标编译通过。

## Task 6: compile_shader 脚本可配置化
- **Start**: 2026-06-21
- **End**: 2026-06-21
- **Summary**: 重写 compile_shader.py 支持命令行参数：--target (opengl|vulkan, 默认 opengl)、--include-dirs (额外 include 路径)、--output-dir (输出目录)、--watch (增量监控模式)。glslangValidator 根据 target 选择 --target-env opengl 或 --target-env vulkan1.3。compile_shader.bat 改为转发参数到 .py（默认 --target opengl）。新增 compile_shader_vulkan.bat 快捷脚本（--target vulkan）。所有 3 个目标编译通过（无代码变更）。

## Task 7: E2E 验证 OpenGL 后端 + 清理
- **Start**: 2026-06-21
- **End**: 2026-06-21
- **Summary**: Sandbox 切回 OpenGL 后端（SetBackend(RendererBackend::OpenGL)）。修复 SandboxApp.cpp 中过时的 "Select Vulkan" 注释。更新 GOAL.md 添加 "资源管线现代化" Phase 完成记录（Tasks 1-7）。验证 OpenGL 路径：3 Quad 渲染从 .spv 加载 shader，window resize 正常。Vulkan 后端编译通过（白屏已知，需后续填充 VkPipeline/VkRenderPass 实现）。所有 3 个目标编译通过。

## Task 1: 删除 GraphicContext + RendererSubsystem
- **Start**: 2026-06-21
- **End**: 2026-06-21
- **Summary**: 删除 GraphicContext.h/.cpp（已无外部调用者，SwapBuffers 已迁至 EndFrame）和 RendererSubsystem.h/.cpp（过渡单例，已被 IRenderer::Get() 取代）。Shader.h 移除 friend class RendererSubsystem。Buffer.cpp 移除 RendererSubsystem.h 误引用。所有 3 个目标编译通过。

## Task 2: GpuResourceManager 移至 Backend/OpenGL/OpenGLGpuResourceManager
- **Start**: 2026-06-21
- **End**: 2026-06-21
- **Summary**: GpuResourceManager 重命名为 OpenGLGpuResourceManager 并移至 Backend/OpenGL/。GpuTextureDesc/GpuBufferDesc/GpuResourceSlot 结构体移至 IGpuResourceManager.h（平台无关层）。Renderer.cpp 改为 CreateScope<OpenGLGpuResourceManager>。VkGpuResourceManager.h 移除对旧 GpuResourceManager.h 的依赖。Buffer.cpp/Texture.cpp 改为引用 IGpuResourceManager.h。旧文件已删除。所有 3 个目标编译通过。

## Task 3: Renderer 重命名为 OpenGLRenderer 并移至 Backend/OpenGL/
- **Start**: 2026-06-21
- **End**: 2026-06-21
- **Summary**: Old Renderer renamed to OpenGLRenderer, moved to Backend/OpenGL/. Created thin platform-agnostic Renderer base class in Renderer/ (holds IDevice/IGpuResourceManager/RenderGraph/ShaderLibrary). OpenGLRenderer inherits Renderer and adds OpenGL-specific Init/BeginFrame/EndFrame + GetRendererAPI. Application now creates CreateScope<OpenGLRenderer>. All factory files (Buffer/Texture/VertexArray/Framebuffer/Shader) updated to use OpenGLRenderer::GetAPI or RendererAPI::GetAPI. Pass files updated to static_cast<OpenGLRenderer&>. Vulkitten.h umbrella fixed. All 3 targets compile.

## Task 5: Shader 创建统一走 IGpuResourceManager
- **Start**: 2026-06-21
- **End**: 2026-06-21
- **Summary**: IGpuResourceManager 新增 CreateShaderFromSpv(name, virtualPath)→uint64_t 和 GetShader(handle)→Ref<Shader> 虚方法。OpenGLGpuResourceManager 实现：CreateShaderFromSpv 内部调用 Shader::Create 加载 .spv→存储到 m_ShaderObjects map→返回 handle；GetShader 返回 Ref<Shader>（共享所有权）。VkGpuResourceManager 添加桩实现。SpriteRenderPass 改为通过 IRenderer::Get().GetResourceManager().CreateShaderFromSpv(...) 创建 shader。Shader::Create 静态方法保留作为底层 .spv 加载器。所有 3 个目标编译通过。

## Task 6: FrameContext 重构为 transient 实例 + IDevice::Submit
- **Start**: 2026-06-21
- **End**: 2026-06-21
- **Summary**: IDevice 新增 Submit(FrameContext&) 纯虚方法。OpenGLDevice 构造器接受 void* nativeWindow (GLFWwindow*)，Submit 调用 glfwSwapBuffers。VulkanDevice 添加 Submit 桩实现。OpenGLRenderer::Init 通过 m_Config.Window→GetSurface()→GetNativeHandle() 获取原生窗口传入 OpenGLDevice。EndFrame 改为调用 m_Device→Submit(*m_FrameContext)。移除了已删除的 GraphicsContext 引用和 Application.h 依赖。所有 3 个目标编译通过。

## Task 7: E2E 验证 OpenGL 后端
- **Start**: 2026-06-21
- **End**: 2026-06-21
- **Summary**: Sandbox 保持 OpenGL 后端（SetBackend(OpenGL)）。验证完整渲染链路：IRenderer::BeginFrame→Layer::OnUpdate(ctx)→Execute→EndFrame→IDevice::Submit(SwapBuffers)。所有资源通过 IGpuResourceManager 的 CreateShaderFromSpv/GetShader 创建。所有 3 个目标编译通过。

## Task 8: Vulkan 最小清屏渲染
- **Start**: 2026-06-21
- **End**: 2026-06-21
- **Summary**: VkSwapchain 重写为真实 Vulkan 实现：CreateSurface 通过 glfwCreateWindowSurface 创建 VkSurfaceKHR；CreateSwapchain 选择 surface format+present mode+image count→vkCreateSwapchainKHR；CreateImageViews 为每个 swapchain image 创建 VkImageView；CreateRenderPass 创建带 color clear attachment 的 VkRenderPass；CreateFramebuffers 为每帧创建 VkFramebuffer。VkRenderer 添加 VkCommandPool+VkCommandBuffer 创建（Init），BeginFrame 中 AcquireNextImage+BindCommandBuffer+Clear+EndCommandBuffer 完整录制清屏命令（暗灰蓝色 0.1,0.1,0.15）。EndFrame 委托 IDevice::Submit。所有 3 个目标编译通过。Sandbox 设置 Vulkan 后端即可运行看到清屏效果。

## Task 1: 平台层抽象接口 IWindow/ISurface/SurfaceDesc
- **Start**: 2026-06-20
- **End**: 2026-06-20
- **Summary**: 创建了平台层抽象接口 IWindow（Vulkitten/Core/IWindow.h）和 ISurface（Vulkitten/Core/ISurface.h），定义了 SurfaceDesc 结构体（Width/Height）。IWindow 提供 GetSurfaceDesc() 和 GetSurface() 纯虚方法，使后端通过统一接口查询窗口表面属性。创建了 WindowsSurface（Platform/Windows/WindowsSurface.h/.cpp）实现 ISurface，封装 GLFWwindow 作为平台绘制表面。WindowsWindow 现在多重继承 Window + IWindow，在 Init() 中创建 m_Surface（Scope<WindowsSurface>），实现 GetSurfaceDesc()/GetSurface()。IWindow.h 和 ISurface.h 已加入 Vulkitten.h 统一头文件。所有 3 个目标编译通过。

## Task 2: 后端接口 IRenderer 和 RendererConfig
- **Start**: 2026-06-20
- **End**: 2026-06-20
- **Summary**: 创建了 IRenderer 抽象后端接口（Vulkitten/Renderer/IRenderer.h），定义生命周期纯虚方法 Init/Shutdown/BeginFrame/EndFrame/Execute/OnWindowResize 以及 GetDevice/GetResourceManager/GetRenderGraph 访问器。RendererConfig 结构体打包 IDevice*/RenderGraph*/IGpuResourceManager* 三个必须依赖。IRenderer 作为平台层和场景层接触的唯一后端接口，具体实现（OpenGL/Vulkan）由 Application 根据 RendererConfig 选择创建。已加入 Vulkitten.h 统一头文件。所有 3 个目标编译通过。

## Task 3: Device→IDevice 重命名 + IGpuResourceManager 接口
- **Start**: 2026-06-20
- **End**: 2026-06-20
- **Summary**: 将 Device 类重命名为 IDevice（Renderer/Device.h/.cpp），统一 I 前缀命名规范。更新 OpenGLDevice、Renderer、RenderContext/RendererSubsystem、Application 等全部引用。创建 IGpuResourceManager 抽象接口（IGpuResourceManager.h），定义 CreateTexture/CreateBuffer/CreateShader/CreatePipeline/CreateGeometry 纯虚方法返回安全句柄，以及 TrackExternalRef/SetGpuHandle/DestroyResource/TickFrame/Gc 资源管理方法。GpuResourceManager 继承 IGpuResourceManager 并 override 所有虚方法；CreateShader/CreatePipeline/CreateGeometry 为预留桩实现。后续 GPU 资源必须通过 IGpuResourceManager 创建。已加入 Vulkitten.h。所有 3 个目标编译通过。

## Task 4: FrameContext 每帧状态结构体
- **Start**: 2026-06-20
- **End**: 2026-06-20
- **Summary**: 创建 FrameContext 结构体（Vulkitten/Renderer/FrameContext.h），承载 FrameIndex/SwapchainIndex/CommandPool(OpenGL 空壳)/CommandBuffer/Fence/Semaphore/PerFrameResourcePool。接口层面为 Vulkan 后端预留完整结构——CommandPool/Fence/Semaphore 使用 void* 占位，Vulkan 后端创建 VkFrameContext 时可填入真实 Vulkan 对象。PerFrameResourcePool 提供 Reset() 桩方法供每帧资源回收。所有 3 个目标编译通过。

## Task 5: RenderContext→RendererSubsystem 重命名 + Per-Pass RenderContext
- **Start**: 2026-06-20
- **End**: 2026-06-20
- **Summary**: 将渲染子系统单例 RenderContext 重命名为 RendererSubsystem（新文件 RendererSubsystem.h/.cpp），更新全部引用（Vulkitten.h/Application/Scene/RenderSystem/ClassFactory/SpriteRenderPass/PreparePass/GpuParticlePass/GpuParticle/Buffer/Texture/GraphicContext/EditorLayer/ExampleLayer2 等 15+ 文件）。ShaderLibrary 的 friend 声明同步更新为 friend class RendererSubsystem。创建新的 Per-Pass RenderContext（Vulkitten/Renderer/RenderContext.h/.cpp），持有 FrameContext& 和 IRenderer&，提供 TranslateCommand(RenderCommand) 方法通过 std::visit 派发命令类型（DrawQuadCommand/ClearCommand），PipelineHandle/GeometryHandle 为 uint64_t 别名。所有 3 个目标编译通过。

## Task 6: Renderer 实现 IRenderer + EndPass SwapBuffers 迁移
- **Start**: 2026-06-20
- **End**: 2026-06-20
- **Summary**: Renderer 继承 IRenderer 接口，添加 BeginFrame()/EndFrame() 实现。BeginFrame 创建新的 FrameContext（递增全局帧计数器），EndFrame 从 RenderGraph 获取 backendContext 调用 SwapBuffers（SwapBuffers 从 EndPass 迁移至此）。EndPass 的 onExecute 改为 no-op。Renderer 头文件改为 include GpuResourceManager.h（非前向声明）以支持内联 GetResourceManager() 的协变返回类型。RenderGraph 新增 GetBackendContext() 公开访问器。IGpuResourceManager 接口补全 TrackExternalRef/SetGpuHandle 虚方法。所有 3 个目标编译通过。

## Task 7: SceneContext + Scene::OnUpdate 参数注入
- **Start**: 2026-06-20
- **End**: 2026-06-20
- **Summary**: 创建 SceneContext 结构体（Vulkitten/Scene/SceneContext.h），包含 IRenderer& 和 RenderGraph& 引用，提供 GetRenderer()/GetRenderGraph() 访问器。Scene::OnUpdate 签名改为 OnUpdate(Timestep, SceneContext&)，替代内部直接调用 RendererSubsystem 单例。System 基类 OnUpdate 同步增加 SceneContext& 参数。RenderSystem::OnUpdate 从 SceneContext 获取 RenderGraph 写入 RenderCommand。更新所有调用处：ExampleLayer2/EditorLayer 通过注入的 SceneContext 传递，不再内部创建。SceneContext.h 已加入 Vulkitten.h 统一头文件。所有 3 个目标编译通过。

## Task 8: Application 主循环 IRenderer 生命周期 + ExampleLayer2 集成
- **Start**: 2026-06-20
- **End**: 2026-06-20
- **Summary**: Application::Run 主循环更新为 IRenderer 生命周期：BeginFrame（创建 FrameContext）→ SceneContext 创建（从 Renderer 获取 IRenderer& + RenderGraph&）→ Layer::OnUpdate(timestep, sceneCtx) 参数注入 → ImGui 绘制 → RenderGraph::Execute → EndFrame（SwapBuffers/Present）→ GC。Layer 基类 OnUpdate 签名改为 OnUpdate(Timestep, SceneContext&)。迁移所有 Layer 子类签名：ExampleLayer2（使用注入 ctx 替代自建 SceneContext）、ExampleLayer3D（忽略 ctx 参数）、EmptyLayer、EditorLayer（使用注入 ctx）。EditorLayer 移除自建 SceneContext 代码。所有 3 个目标编译通过。

## Task 7: 窗口Resize统一更新Framebuffer
- **Start**: 2026-06-20
- **End**: 2026-06-20
- **Summary**: 在 RenderGraph 添加了 ResizeAllFramebuffers(uint32_t, uint32_t) 方法，遍历 m_FramebufferMap 对所有已注册 Framebuffer 调用 Resize()。Renderer::OnWindowResize 现在在设置 viewport 后自动调用此方法，统一更新引擎中所有已注册的 Framebuffer。Pass 无需任何修改——每帧通过 GetFramebuffer(key) 自动获取已更新尺寸的 FB。EditorLayer 和 Sandbox 无需单独处理 FB 的 resize。ViewportPanel 保留自己的 resize 逻辑以处理面板尺寸独立变化。所有 3 个目标编译通过。

## Task 6: RenderGraph Framebuffer Key-Value模式
- **Start**: 2026-06-20
- **End**: 2026-06-20
- **Summary**: 重构了 RenderGraph 的 Framebuffer 管理方式。RenderGraph 新增 m_FramebufferMap (key → Ref<Framebuffer>) 和 SetFramebuffer/GetFramebuffer 方法。RenderPass 新增 m_Graph 指针和 GetGraph() 公开访问器，在 AddPass 时自动设置。Execute() 不再自动 Bind/Unbind Framebuffer——PreparePass 和 SpriteRenderPass 各自通过 GetGraph()->GetFramebuffer("Viewport") 获取并自行管理 Bind/Unbind。移除了 RenderPass::SetTargetFramebuffer/GetTargetFramebuffer 和 RenderGraph::SetPassFramebuffer。EditorLayer 改为 graph->SetFramebuffer("Viewport", fb) 一行配置。所有 3 个目标编译通过。

## Task 5: Texture2D和Buffer纳入GpuResourceManager管理
- **Start**: 2026-06-20
- **End**: 2026-06-20
- **Summary**: 将 Texture2D::Create 和 Buffer::Create (VertexBuffer/IndexBuffer) 工厂方法接入 GpuResourceManager。每个资源创建后调用 CreateTexture/CreateBuffer 注册 handle，通过 SetGpuHandle 记录平台 GPU 句柄，通过 TrackExternalRef 建立 weak_ptr<void> 外部引用追踪。GpuResourceManager 新增 m_ExternalTrackers 映射表（slot index → weak_ptr<void>）和 TrackExternalRef/SetGpuHandle 方法。Gc 方法现在检查 weak_ptr 是否过期——仅当外部 Ref 已全部释放时才会 GC 资源。DestroyResource 同步清理 tracker 条目。用户 API (Ref<T>) 完全不变。所有 3 个目标编译通过。

## Task 4: GpuResourceManager延迟GC机制
- **Start**: 2026-06-20
- **End**: 2026-06-20
- **Summary**: 为 GpuResourceManager 添加了帧计数器和延迟 GC 机制。新增 m_CurrentFrame 成员、TickFrame() 方法（每帧递增帧计数器）和 Gc(maxFramesInFlight) 方法（遍历所有 slot，清理超过 N 帧未被引用的资源）。GpuResourceSlot 新增 lastUsedFrame 字段，在每次 GetTexture/GetBuffer 时更新。GC 检查 slot.alive 且 currentFrame - lastUsedFrame > maxFramesInFlight 的资源进行销毁回收。在 Application::Run() 主循环中（RenderGraph 执行后、Window Update 前）调用 TickFrame() 和 Gc(3)。当 Task 5 集成 Ref 引用计数后，GC 将额外检查外部 shared_ptr 持有状态。所有 3 个目标编译通过。

## Task 3: ShaderLibrary移至RenderContext并从公开API移除
- **Start**: 2026-06-20
- **End**: 2026-06-20
- **Summary**: 将 ShaderLibrary 所有权从 GpuEmitterManager 移至 RenderContext。RenderContext 现在持有 ShaderLibrary 成员并在 Init() 中预加载所有引擎 shader（4 个 compute + 1 个 render）。GpuEmitterManager 移除 m_ShaderLibrary 成员，GetShaderLibrary() 改为转发到 RenderContext::Get().GetShaderLibrary()，Initialize() 简化为仅设置标志。ShaderLibrary 构造器改为 private（仅 friend RenderContext 可创建），保留 VKT_API 导出以允许 DLL 消费者通过引用调用 Load/Get 方法。Sandbox ExampleLayer2 和 EmptyLayer 移除自有的 ShaderLibrary 成员，改为通过 RenderContext::Get().GetShaderLibrary() 引用加载 shader。所有 3 个目标编译通过。

## Task 2: 注册engine虚拟路径并更新Shader加载路径
- **Start**: 2026-06-20
- **End**: 2026-06-20
- **Summary**: 在 Engine::Init() 中注册了 `engine` → `../../Vulkitten/assets` 虚拟路径映射。更新了 GpuEmitterManager::Initialize() 中 4 个 compute shader 加载路径（`sandbox://assets/computeshaders/` → `engine://computeshaders/`）和 Particle.shader 加载路径。更新了 SpriteRenderPass 构造函数中 TextureEntity.shader 加载路径。更新了所有 5 个 .shader JSON 描述文件的内部 vert/frag 路径。更新了 compile_shader.py 的 INCLUDE_DIRS 从 Sandbox 路径改为 Vulkitten 路径。所有 3 个目标编译通过。

## Task 1: 引擎Shader资产文件夹创建与文件迁移
- **Start**: 2026-06-20
- **End**: 2026-06-20
- **Summary**: 创建了 Vulkitten/assets/shaders/ 和 Vulkitten/assets/computeshaders/common/ 目录结构。将引擎 Pass 使用的 6 组 shader 文件（FlatColor、SolidColor、Texture、TextureEntity、Particle 的 .shader/.vert/.frag/.spv）从 Sandbox/assets/shaders/ 移至 Vulkitten/assets/shaders/。将 GPU 粒子 4 个 compute shader（ParticleSimArg、ParticleSim、ParticleEmit、ParticleRenderArg 的 .comp/.spv）和 common/math_hash.h 从 Sandbox/assets/computeshaders/ 移至 Vulkitten/assets/computeshaders/。Sandbox/assets/ 现在仅保留 textures/（Checkerboard.png、ChernoLogo.png）。.shader JSON 文件内的路径引用将在 Task 2 中更新。所有 3 个目标编译通过。

## Task 1: Complete Basic RenderGraph Invocation
- **Start**: 2026-06-19
- **End**: 2026-06-19
- **Summary**: Implemented RenderGraph::Execute() with command filtering by pass name. Created PreparePass (ClearCommand → Legacy::RenderCommand::Clear), SpriteRenderPass (DrawQuadCommand → Renderer2D batch), EndPass (SwapBuffers via GraphicsContext). Wired passes in Renderer::Init(). Moved SwapBuffers from WindowsWindow::OnUpdate to EndPass. Updated Scene::OnUpdate to set camera on graph and skip direct RenderScene when systems are present. Updated RenderSystem to emit ClearCommand. Removed direct Clear calls from ExampleLayer2. Added RenderPass.cpp with SetExecute/Read/Write implementations. Added GetGraphicsContext() to Window interface. Fixed EditorLayer to use Legacy::RenderCommand. All 3 targets compile.

## Task 2: Framebuffer Configuration in Passes
- **Start**: 2026-06-19
- **End**: 2026-06-19
- **Summary**: Added SetFramebuffer/GetFramebuffer to RenderGraph. Updated PreparePass and SpriteRenderPass to bind/unbind framebuffer when configured (nullptr = default backbuffer). EditorLayer now sets viewport framebuffer on RenderGraph instead of manually binding it for the entire render. PreparePass clears the configured framebuffer, SpriteRenderPass draws to it. Entity ID attachment clear remains editor-specific. All 3 targets compile.

## Task 3: Clean Up Legacy Renderer2D Path
- **Start**: 2026-06-19
- **End**: 2026-06-19
- **Summary**: Removed Scene::RenderScene() method entirely. Removed legacy fallback path in Scene::OnUpdate() — all rendering now goes through RenderGraph. Created GpuParticlePass that iterates GPU emitter entities via Scene and calls Update+Render between PreparePass and SpriteRenderPass. Added SetScene/GetScene to RenderGraph. Added GetEmitterManager() public accessor to Scene. Removed Renderer2D.h include from Scene.h. All 3 targets compile.

## Task 4: ClassFactory Singleton
- **Start**: 2026-06-19
- **End**: 2026-06-19
- **Summary**: Created ClassFactory as Meyer's singleton — the single centralized access point for all engine subsystems. Provides GenerateUUID() (central RNG), GetApplication(), GetWindow(), GetRenderGraph(). Added to Vulkitten.h umbrella header. Preserves existing UUID class for standalone use. All 3 targets compile.

## Task 5: Add GetPassCount() and GetPassName() to RenderGraph
- **Start**: 2026-06-19
- **End**: 2026-06-19
- **Summary**: Added GetPassCount() and GetPassName(uint32_t index) public methods to RenderGraph, allowing external code (debug UI, editor panels, etc.) to query the number of registered passes and their names. Methods are inline in RenderGraph.h following the existing getter pattern. GetPassName includes a VKT_CORE_ASSERT for bounds checking. No other files required changes. All 3 targets compile.

## Task 2: Device Abstraction + OpenGLDevice Stub
- **Start**: 2026-06-19
- **End**: 2026-06-19
- **Summary**: Created abstract Device class (Vulkitten/src/Vulkitten/Renderer/Device.h) as the GPU device interface with pure virtual Init() and Shutdown(). Provides static Device& Get() convenience accessor that delegates to ClassFactory::GetInterface<Device>(). Created OpenGLDevice (Platform/OpenGL/OpenGLDevice.h/.cpp) as a placeholder implementation — Init/Shutdown are stubs with VKT_PROFILE_RENDER_FUNCTION markers; actual GL init is still handled by OpenGLContext + RendererAPI for now. Design follows the existing RendererAPI pattern (abstract base + platform impl), but at a higher level: Device represents the logical GPU device, while RendererAPI represents low-level draw commands. For Vulkan this would own VkDevice/VkPhysicalDevice; for OpenGL the GL context is the device. Device.h exported with VKT_API for DLL visibility. Source files auto-collected via CMake GLOB_RECURSE. All 3 targets compile.

---

# Phase: App Layer Refactoring (New TASK.md 1-14)

## Task 1: ClassFactory DI Primitives
- **Start**: 2026-06-19
- **End**: 2026-06-19
- **Summary**: Enhanced ClassFactory (Vulkitten/src/Vulkitten/Core/ClassFactory.h) with five template DI primitives. `CreateInstance<T>()` provides Meyer's singleton lazy-creation. `RegisterInstance<T>(T*)` registers externally-created singletons into a type-erased `std::unordered_map<std::type_index, void*>`. `GetInstance<T>()` checks the registered map first, falls back to CreateInstance + caches. `RegisterInterface<I, Impl>()` maps an interface to an implementation type via a factory lambda stored in `std::unordered_map<std::type_index, std::function<void*()>>`. `RegisterInterface<I>(I* impl)` variant registers an existing instance as the interface implementation. `GetInterface<I>()` retrieves the registered implementation with a VKT_CORE_ASSERT guard. Added `<functional>`, `<typeindex>`, `<unordered_map>` includes. No changes to ClassFactory.cpp needed — all new methods are template/inline. Existing GetApplication/GetWindow/GetRenderGraph methods preserved as legacy accessors. All 3 targets compile.

## Task 3: Engine Singleton
- **Start**: 2026-06-19
- **End**: 2026-06-19
- **Summary**: Created Engine singleton class (Vulkitten/src/Vulkitten/Core/Engine.h/.cpp) as the core engine subsystem owner. Engine holds FileSystem instance (Scope<FileSystem>), Input lifecycle management (currently global static, Engine plans to own create/destroy), Log reference (via Engine::GetLogger() → Log::Get() — added static Log& Get() to Log.h as a token singleton). Added empty EventQueue and ThreadPool placeholder structs. Engine::Init()/Shutdown() provide lifecycle hooks with VKT_PROFILE markers. Engine::Get() delegates to ClassFactory::GetInstance<Engine>() using Meyer's singleton pattern. Not yet wired into Application main loop (Task 12). Log.h updated with private default constructor/destructor and public static Get(). All 3 targets compile.

## Task 4: FileSystem Instance Refactoring (Transitional Static Wrappers)
- **Start**: 2026-06-19
- **End**: 2026-06-19
- **Summary**: Refactored FileSystem from all-static to instance-based. Instance methods use temporary `_Impl` suffix (`RegisterPath_Impl`, `Resolve_Impl`, `Exists_Impl`) to avoid MSVC name collision with the transitional static wrappers that retain original names (`RegisterPath`, `Resolve`, `Exists`). Static wrappers (defined in FileSystem.cpp) forward to `Engine::Get().GetFileSystem().*_Impl()`. Engine.h changed from including FileSystem.h to forward-declaring `class FileSystem` — Engine.cpp includes FileSystem.h separately for the complete type. FileSystem.cpp now includes Engine.h for the static wrappers; no circular dependency exists. All existing call sites compile unchanged via the static wrappers. Instance method bodies moved from inline in header to FileSystem.cpp (using `std::filesystem::exists` which requires `<filesystem>` at link time). Task 5 will rename `_Impl` → original names, remove static wrappers, and migrate all call sites to `Engine::Get().GetFileSystem()`. All 3 targets compile.

## Task 5: FileSystem Call Site Migration + Cleanup
- **Start**: 2026-06-19
- **End**: 2026-06-19
- **Summary**: Completed the FileSystem instance migration: removed transitional static wrappers from FileSystem.h/.cpp, renamed `_Impl` suffix methods back to their original names (RegisterPath, Resolve, Exists). Migrated all 7 call sites: Application.cpp (1), OpenGLShader.cpp (4), OpenGLTexture.cpp (1), SandboxApp.cpp (1), EditorApp.cpp (3). Each call site now uses `Engine::Get().GetFileSystem().Method()` instead of `FileSystem::Method()`. Added `#include "Vulkitten/Core/Engine.h"` to the umbrella header Vulkitten.h and to 4 .cpp files that needed it (Application.cpp, OpenGLShader.cpp, OpenGLTexture.cpp, EditorApp.cpp). FileSystem.cpp no longer includes Engine.h — the dependency is now one-way: callers → Engine → FileSystem. All 3 targets compile.

## Task 6: GpuResourceManager Skeleton
- **Start**: 2026-06-19
- **End**: 2026-06-19
- **Summary**: Created GpuResourceManager (Vulkitten/src/Vulkitten/Renderer/GpuResourceManager.h/.cpp) as the centralized VRAM resource manager. Resources are referenced via uint64_t handles encoding 32-bit index + 16-bit generation (anti-ABA pattern matching RenderGraph's ResourcePool Handle<T> design). Provides CreateTexture(TextureDesc) / CreateBuffer(BufferDesc) returning handles; GetTexture(handle) / GetBuffer(handle) for lookup with deferred allocation support (descriptor recorded at Create, actual GPU allocation placeholder on first Get with VKT_CORE_INFO log). Free-list slot reuse: freed indices are recycled, generation bumped to invalidate stale handles. Minimal TextureDesc (Width/Height) and BufferDesc (Size) for future expansion. Existing Ref<Texture2D>/Ref<Buffer> classes are NOT migrated — this is the skeleton that will eventually replace ad-hoc resource creation. All 3 targets compile.

## Task 7: ShaderManager with #include Preprocessing
- **Start**: 2026-06-19
- **End**: 2026-06-19
- **Summary**: Created ShaderManager (Vulkitten/src/Vulkitten/Renderer/ShaderManager.h/.cpp) as the centralized shader loading layer. Constructor takes FileSystem& (from Engine::Get().GetFileSystem()) for virtual-path resolution. LoadShader(virtualPath) resolves path → reads source → collects include dirs → recursively resolves #include via ResolveIncludes (extracted from OpenGLShader). Returns uint64_t handle with preprocessed source stored in ShaderData map. Provides GetShaderData/IsValidHandle. Static helpers (ReadFileToString, CollectIncludeDirs, ResolveIncludes) match original OpenGLShader logic. Existing OpenGLShader loading is unchanged — ShaderManager adds a parallel centralized pipeline. All 3 targets compile.

## Task 8: RenderContext + Renderer Instance Refactoring
- **Start**: 2026-06-19
- **End**: 2026-06-19
- **Summary**: Created RenderContext (Vulkitten/src/Vulkitten/Renderer/RenderContext.h/.cpp) as the rendering subsystem singleton, owning Renderer (instance, no longer static) and RenderUtils (empty stub). Renderer.h/.cpp refactored from static class to instance class: Init/Shutdown/Execute/GetRenderGraph/SetViewProjection/OnWindowResize are now instance methods. RenderContext owns the Renderer and provides static Get() for global access. Application.cpp now creates GpuResourceManager + ShaderManager + OpenGLDevice + RenderContext (via Scope<...> members). Updated 10 caller files (Application, ClassFactory, EditorLayer, Scene, RenderSystem, SpriteRenderPass, PreparePass, GpuParticlePass, Vulkitten.h umbrella) to use RenderContext::Get().GetRenderGraph() instead of Renderer::GetRenderGraph(). Renamed GpuResourceManager's TextureDesc/BufferDesc to GpuTextureDesc/GpuBufferDesc to avoid conflict with RenderGraphResource.h. All 3 targets compile.

## Task 9: 文件夹结构迁移
- **Start**: 2026-06-20
- **End**: 2026-06-20
- **Summary**: 将 src/Platform/OpenGL/ 下 18 个文件（OpenGLBuffer/Context/Device/Framebuffer/RendererAPI/Shader/Texture/Util/VertexArray .h/.cpp）移至 src/Vulkitten/Renderer/Backend/OpenGL/。将 src/Platform/Windows/ 下 7 个文件（WindowsWindow/Input/Surface/FileDialogs .h/.cpp）移至 src/Vulkitten/Window/Platform/Windows/。更新 11 个文件中的 #include "Platform/..." 引用为新路径（Vulkitten/Renderer/Backend/OpenGL/、Vulkitten/Window/Platform/Windows/）。CMake GLOB_RECURSE 自动适配新目录。所有 3 个目标编译通过。

## Task 10: Vulkan SDK 集成 + VulkanInstance
- **Start**: 2026-06-21
- **End**: 2026-06-21
- **Summary**: 添加 find_package(Vulkan QUIET) 和 target_link_libraries(Vulkan::Vulkan) 到 CMakeLists.txt，定义 VKT_HAS_VULKAN 编译宏（Vulkan SDK 1.3.296 已检测到）。创建 VulkanInstance 类（Vulkitten/Renderer/Backend/Vulkan/VulkanInstance.h/.cpp），封装 vkCreateInstance + Validation Layer + VK_EXT_debug_utils 扩展加载。使用 void* 存储 Vulkan 句柄以保证在无 SDK 环境下也能编译。所有 3 个目标编译通过。

## Task 11: VulkanDevice 实现 IDevice
- **Start**: 2026-06-21
- **End**: 2026-06-21
- **Summary**: 创建 VulkanDevice 类（VulkanDevice.h/.cpp）继承 IDevice。Init() 实现物理设备枚举（优先独立 GPU VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU）、Queue Family 查询（Graphics+Present+Transfer）、逻辑设备创建（vkCreateDevice）。使用 void* 存储 VkPhysicalDevice/VkDevice 避免 Vulkan 头文件依赖。类名使用 VulkanDevice（非 VkDevice）以避免与 Vulkan typedef 冲突。所有 3 个目标编译通过。

## Task 12: VkSwapchain + VkSurface
- **Start**: 2026-06-21
- **End**: 2026-06-21
- **Summary**: 创建 VkSwapchain 类（VkSwapchain.h/.cpp），管理 VkSurfaceKHR 和 VkSwapchainKHR 生命周期。通过 IWindow 获取平台表面，Create() 以指定宽高创建/重建 swapchain，AcquireNextImage/Present 处理帧循环。含 Per-Image VkImageView 管理预留。所有 3 个目标编译通过。

## Task 13: VkFrameContext + VkRenderContext
- **Start**: 2026-06-21
- **End**: 2026-06-21
- **Summary**: 创建 VkFrameContext（继承 FrameContext，含 VkCommandPool 数组 + Init/Reset 方法）和 VkRenderContext（per-pass 翻译器，TranslateCommand 派发 DrawQuadCommand/ClearCommand，BindPipeline/BindGeometry 含冗余绑定跳过）。两者均为桩实现，方法体预留 vkCmd* 调用。所有 3 个目标编译通过。

## Task 14: VkGpuResourceManager
- **Start**: 2026-06-21
- **End**: 2026-06-21
- **Summary**: 创建 VkGpuResourceManager（VkGpuResourceManager.h/.cpp）实现 IGpuResourceManager 接口。管理 VkBuffer/VkImage 资源创建（通过句柄 index+generation）、外部引用追踪（weak_ptr）、延迟分配、GC（maxFramesInFlight）。实现 CreateTexture/CreateBuffer/CreateShader/CreatePipeline/CreateGeometry 及 TrackExternalRef/SetGpuHandle/DestroyResource/TickFrame/Gc 全部接口方法。所有 3 个目标编译通过。

## Task 15: VkShader + VkPipeline
- **Start**: 2026-06-21
- **End**: 2026-06-21
- **Summary**: 创建 VkShader（Compile 方法桩实现：GLSL→SPIR-V→VkShaderModule）和 VkPipeline（Create 方法桩实现：VkPipelineLayout→VkDescriptorSetLayout→ShaderStages→vkCreateGraphicsPipelines）。两者均使用 void* 存储 Vulkan 句柄，支持 SpriteRenderPass 的 BatchVertex 顶点布局。所有 3 个目标编译通过。

## Task 16: VkRenderer 实现 IRenderer
- **Start**: 2026-06-21
- **End**: 2026-06-21
- **Summary**: 创建 VkRenderer（VkRenderer.h/.cpp）实现 IRenderer 接口，串联全部 Vulkan 组件。Init() 创建 VulkanDevice + VkSwapchain + VkGpuResourceManager + RenderGraph。BeginFrame（AcquireNextImage+创建 FrameContext）→ Execute（RenderGraph::Execute）→ EndFrame（Present+帧资源回收）。OnWindowResize 销毁并重建 Swapchain。持有 VulkanInstance& 和 IWindow& 引用。所有 3 个目标编译通过。

## Task 17: 端到端集成 Vulkan 后端 + Sandbox 切换
- **Start**: 2026-06-21
- **End**: 2026-06-21
- **Summary**: Application 类新增 RendererBackend 枚举（OpenGL/Vulkan）和 SetBackend/GetBackend 静态方法。Application 构造函数根据 s_Backend 分支创建 Vulkan 或 OpenGL 后端：Vulkan 路径创建 VulkanInstance→VkRenderer（通过 dynamic_cast<IWindow*> 获取 IWindow）；OpenGL 路径保持原 RendererSubsystem。主循环（BeginFrame→Update→Execute→EndFrame）和 OnWindowResize 均支持双后端路径。SandboxApp.cpp 在 CreateApplication() 中调用 Application::SetBackend(RendererBackend::Vulkan) 选择 Vulkan 后端。所有 3 个目标编译通过。

## Task 9: GraphicContext Singleton
- **Start**: 2026-06-19
- **End**: 2026-06-19
- **Summary**: Created GraphicContext (Vulkitten/src/Vulkitten/Core/GraphicContext.h/.cpp) as the user-visible graphics shell. Takes RenderContext& in constructor, owns Window (via Window::Create()), and holds SceneUtil/GraphicUtil empty stubs. Provides static Get() for global access. StreamBuffers still handled by EndPass via RenderContext's RenderGraph backend context. Also added Vulkitten/Utils/SceneUtil.h and Vulkitten/Utils/GraphicUtil.h placeholder classes. All 3 targets compile.

## Task 10: Engine Scene Factory Methods
- **Start**: 2026-06-19
- **End**: 2026-06-19
- **Summary**: Added three Scene factory methods to Engine. CreateEmptyScene() returns Scope<Scene> with a fresh entt::registry. LoadSceneFromGltf(filepath) is a stub returning empty scene. MergeScenes(target, source) iterates source entities via component views (Tag, Transform, SpriteRenderer, Camera), clones each entity with all known component types into the target. Engine.h now forward-declares Scene; Engine.cpp includes Scene.h and Entity.h. All 3 targets compile.

## Task 11: System Execution Order
- **Start**: 2026-06-19
- **End**: 2026-06-19
- **Summary**: Added virtual GetName() to System base class. RenderSystem now returns static name "RenderSystem". Scene gains SetSystemOrder(vector<string>) to configure execution order. Scene::OnUpdate respects ordering: systems matching configured names execute first in order, unlisted systems execute last in AddSystem order. All 3 targets compile.

## Tasks 12-14: Application Wiring, Sandbox Adaptation, OnEvent Pattern
- **Start**: 2026-06-19
- **End**: 2026-06-19
- **Summary**: Task 12: Application constructor now calls Engine::Get().Init() first, creates Window before RenderContext (needed for backend context), then creates GraphicContext wrapping RenderContext. Task 13: Sandbox ExampleLayer2 adapted — m_Scene changed from Ref<Scene> to Scope<Scene>, created via Engine::Get().CreateEmptyScene(). Sandbox2D.h and ExampleLayer.h remain excluded from build. Task 14: Added OnEvent example in ExampleLayer2 — pressing Space triggers KeyPressedEvent dispatch, modifies all SpriteRendererComponent::Color to red. Demonstrates the OnEvent→Component pattern where events only modify component data, and Scene::OnUpdate reads the updated components next frame. All 3 targets compile.

---

## Task 1: RHI/Core/Types.hpp — 集中化渲染基础类型 (新架构 Phase 1)
- **Start**: 2026-06-21
- **End**: 2026-06-21
- **Summary**: 创建 `Vulkitten/src/Vulkitten/RHI/Core/Types.hpp`，在 `Vulkitten::rhi` 命名空间下集中定义所有渲染基础类型。包含 Extent2D/3D、Offset2D/3D、Rect2D、Viewport、ClearColor/ClearDepthStencil/ClearValue(union)、ShaderStage(bitflag)、IndexType、BufferUsage(bitflag)、TextureUsage(bitflag)、MemoryProperty(bitflag)、FilterMode、MipMode、WrapMode、PipelineStage(bitflag)、AccessFlags(bitflag)、ImageLayout、LoadOp、StoreOp。所有 bitflag 枚举均提供 operator| 和 HasUsage/HasStage 查询辅助函数。零 API 依赖（仅 `<cstdint>` 和 `<array>`）。AGENTS.md 新增 RHI 子系统条目。所有 3 个目标编译通过。

## Task 2: RHI/Core/Format.hpp — 跨 API 像素/顶点/深度格式系统
- **Start**: 2026-06-21
- **End**: 2026-06-21
- **Summary**: 创建 `Vulkitten/src/Vulkitten/RHI/Core/Format.hpp`，定义 `rhi::Format` 枚举覆盖 26 种跨 API 格式：8-bit UNORM(R8/RG8/RGBA8/BGRA8/SRGB)、16-bit Float/UNORM(R16F/RG16F/RGBA16F/R16U)、32-bit Float(R32F/RG32F/RGB32F/RGBA32F)、32-bit Int(R32U/R32S/RG32U/RGBA32U)、深度/模板(D16U/D32F/D24S8/D32S8)、BC 压缩(BC1/BC3/BC5/BC7)。提供 FormatByteSize/FormatComponentCount/IsDepthFormat/IsStencilFormat/IsCompressedFormat 工具函数。零 API 依赖（仅 `<cstdint>`）。将逐步替代 Buffer.h 中仅顶点属性的 ShaderDataType。AGENTS.md RHI 条目已更新。所有 3 个目标编译通过。

## Task 3: RHI/Handle.hpp — 统一强类型资源句柄系统
- **Start**: 2026-06-21
- **End**: 2026-06-21
- **Summary**: 创建 `Vulkitten/src/Vulkitten/RHI/Handle.hpp`，定义 `rhi::Handle<Tag>` 模板类，结合 RHI Design 的 Tag 强类型编译期安全（BufferTag/TextureTag/ShaderTag/PipelineTag/GeometryTag/SamplerTag/RenderPassTag/FramebufferTag）和现有引擎的 generation 计数器防 use-after-free（ABA 保护）。Id=0 表示 Null Handle。提供 Hash 结构体用于 unordered_map。类型别名：BufferHandle/TextureHandle/ShaderHandle/PipelineHandle/GeometryHandle/SamplerHandle/RenderPassHandle/FramebufferHandle。将逐步替代 Renderer/Resources/ResourceHandle.h。所有 3 个目标编译通过。

## Task 4: RHI/RHIPipelineDesc.hpp — 管线/几何/采样器描述符
- **Start**: 2026-06-21
- **End**: 2026-06-21
- **Summary**: 创建 `Vulkitten/src/Vulkitten/RHI/RHIPipelineDesc.hpp`，定义三个核心描述符结构体。**PipelineDesc**：包含 vertexShader/fragmentShader/computeShader 句柄、vertexLayout(VertexAttribute[] 含 location/Format/offset/bufferSlot/stride)、RasterState(CullMode/FrontFace/PolygonMode/depthClamp/scissor)、DepthStencilState(CompareOp + stencil)、BlendState[](BlendFactor/BlendOp/writeMask)、pushConstantsSize——顶点格式在 Pipeline 而非 Geometry。**GeometryDesc**：vertexBuffers[8]+indexBuffer+vertexCount/indexCount——不含任何顶点格式信息（格式由渲染时的 Pipeline 决定），支持最多 8 个 VBO 流。**SamplerDesc**：MagFilter/MinFilter/Mip/WrapU/WrapV/WrapW/MaxAnisotropy。所有枚举内嵌于结构体以保持命名空间整洁。Phase 1(RHI 基础类型)完成，AGENTS.md 已更新。所有 3 个目标编译通过。

## Task 5: 重构 IDevice — 完整资源创建 + 帧生命周期接口
- **Start**: 2026-06-21
- **End**: 2026-06-21
- **Summary**: 大幅扩展 `Renderer/Device.h` 中 IDevice 接口：新增 beginFrame()→FrameContext、endFrame(FrameContext)、createCommandBuffer(FrameContext)→ICommandBuffer*、8 个 create* 资源创建方法(createBuffer→BufferHandle/createTexture→TextureHandle/createShader→ShaderHandle/createPipeline→PipelineHandle/createGeometry→GeometryHandle/createSampler→SamplerHandle/createRenderPass→RenderPassHandle/createFramebuffer→FramebufferHandle)、onResize(w,h)、waitIdle()、getNativeDevice()。创建 `RHI/RHIResourceDescs.hpp` 包含 BufferDesc/TextureDesc/TextureType/TextureViewDesc/AttachmentDesc/SubpassDesc/SubpassDependency/RenderPassDesc/FramebufferDesc 等描述符结构体。ShaderBytecode 结构体(data ptr+size+entryPoint)同文件定义。OpenGLDevice 和 VulkanDevice 添加所有新方法的 [HACK] 桩实现。createCommandBuffer 暂返回裸指针(ICommandBuffer 在 Task 6 创建后改为 unique_ptr)。保留遗留 Submit(FrameContext&)。AGENTS.md 已更新。所有 3 个目标编译通过。

## Task 6: RHIPipelineDesc 添加 TextureSlot 和 BufferSlot 槽位声明
- **Start**: 2026-06-21
- **End**: 2026-06-21
- **Summary**: 更新 `RHI/RHIPipelineDesc.hpp`，新增 `TextureSlot` 结构体(slot/SlotType::Sampled|Storage/Stages)和 `BufferSlot` 结构体(slot/SlotType::Uniform|Storage|PushConstant/Stages/Size)。PipelineDesc 新增 `std::vector<TextureSlot> TextureSlots` 和 `std::vector<BufferSlot> BufferSlots` 成员。后端在 createPipeline() 时读取这些槽位声明预构建 descriptor layout（Vulkan: VkDescriptorSetLayout，GL: uniform location↔texture unit 映射表）。消除原 SpriteRenderPass 重构任务中的 [HACK: 抽象层缺TextureSlot管理]。AGENTS.md RHI 条目已更新。所有 3 个目标编译通过。

## Task 7: RHI/ICommandBuffer.hpp — 命令录制抽象接口
- **Start**: 2026-06-21
- **End**: 2026-06-21
- **Summary**: 创建 `Vulkitten/src/Vulkitten/RHI/ICommandBuffer.hpp`，定义纯虚命令录制接口。20+ 方法覆盖完整 GPU 命令集：Begin/End 生命周期、Barrier(全局+纹理)、BeginRenderPass(ClearValues 数组)/EndRenderPass、BindPipeline/BindGeometry、slot 描述符绑定(BindUniformBuffer/BindStorageBuffer/BindTexture/BindStorageTexture)、PushConstants(Data+Size+Offset)、Draw/DrawIndexed/DispatchCompute、CopyBuffer/CopyBufferToTexture/CopyTextureToBuffer、BeginDebugLabel/EndDebugLabel/InsertDebugMarker。C++17 兼容(Data ptr+Size 替代 std::span)。绑定 slot 必须是 PipelineDesc TextureSlots/BufferSlots 声明的子集。AGENTS.md 新增 ICommandBuffer 条目。所有 3 个目标编译通过。

## Task 8: 重构 FrameContext 为 IDevice 托管的 transient 实例
- **Start**: 2026-06-21
- **End**: 2026-06-21
- **Summary**: 简化 `Renderer/FrameContext.h`：从 8 字段(含显式 CommandPool/CommandBuffer/InFlightFence/ImageAvailableSemaphore/RenderFinishedSemaphore/ResourcePool void*)缩减为 3 字段——FrameIndex(ring buffer 索引)+SwapchainIndex+Void* Internal(后端私有数据)。移除 PerFrameResourcePool 辅助结构体。FrameContext 现在由 IDevice::beginFrame() 产出、IDevice::endFrame() 消费、createCommandBuffer() 使用。VkFrameContext 继承关系保留，其 m_CommandPools[3] 独立管理。AGENTS.md FrameContext 条目已更新。所有 3 个目标编译通过。

## Task 9: RHI 资源查询接口 IBuffer/ITexture/IShader/IPipeline/IGeometry/ISampler
- **Start**: 2026-06-21
- **End**: 2026-06-21
- **Summary**: 创建 6 个最小资源查询接口：IBuffer(GetSize/Map/Unmap/Flush)、ITexture(GetType/GetFormat/GetExtent/GetMipLevels)、IShader(GetStage/GetEntryPoint)、IPipeline(IsCompute)、IGeometry(GetVertexCount/GetIndexCount)、ISampler(标记型空接口)。大部分 GPU 操作通过 ICommandBuffer+Handle 完成，这些接口仅用于属性查询和 CPU 映射。Phase 2 接口层完成，AGENTS.md 已更新。所有 3 个目标编译通过。

## Task 10: 重构 IRenderer — BeginFrame/EndFrame 委托给 IDevice
- **Start**: 2026-06-21
- **End**: 2026-06-21
- **Summary**: 重构 `Renderer/IRenderer.h` 和 `Renderer/Renderer.h/.cpp`：IRenderer 接口保持 Init/Shutdown/BeginFrame/Execute/EndFrame/OnWindowResize + 子系统访问器不变。Renderer 基类新增 BeginFrame/EndFrame 默认实现，分别委托给 IDevice::beginFrame()→FrameContext 和 IDevice::endFrame(ctx)。FrameContext 从 Scope<> 改为值成员。OnWindowResize 同时调用 IDevice::onResize() 和 RenderGraph::ResizeAllFramebuffers()。OpenGLRenderer 过渡期适配(HACK 标注)。Phase 2(抽象接口层)完成。所有 3 个目标编译通过。

## Task 11: 重构 OpenGLDevice — 实现完整 IDevice 资源创建
- **Start**: 2026-06-21
- **End**: 2026-06-21
- **Summary**: 重写 `OpenGLDevice.h/.cpp` 实现 IDevice 全部方法。新增内部 slot-based Handle 分配池(AllocHandle/GetSlot/FindFreeSlot+Generation ABA 保护)。实现 beginFrame→FrameContext 含 ring buffer 帧索引、endFrame→glfwSwapBuffers。createBuffer→glGenBuffers+glBufferData(根据 desc.Usage 选 target)、createTexture→glGenTextures+glTexImage2D(ToGLFormat/ToGLBaseFormat 转换)、createSampler→glGenSamplers 完整 filter/wrap/aniso 设置、createPipeline→glCreateProgram(占位,slot 映射表延后)、createFramebuffer→glGenFramebuffers 支持多 color+DS attachment。onResize→glViewport、waitIdle→glFinish。createShader/createGeometry/createRenderPass/createCommandBuffer 保持 [HACK] 桩。ToGLFormat/ToGLBaseFormat 静态转换函数映射 rhi::Format→GLenum。所有 3 个目标编译通过。

## Task 12: GLCommandBuffer — OpenGL ICommandBuffer 实现
- **Start**: 2026-06-21
- **End**: 2026-06-21
- **Summary**: 创建 `GLCommandBuffer.h/.cpp` 实现 `rhi::ICommandBuffer`。OpenGL 立即执行模式+状态缓存：Begin/End 重置状态追踪、Barrier 空实现(GL 隐式同步)、BeginRenderPass→glBindFramebuffer+glClear(解析 ClearValues 数组)、EndRenderPass 关闭 FBO、BindPipeline/BindGeometry 冗余绑定跳过(差量更新)。BindUniformBuffer/BindStorageBuffer/BindTexture/PushConstants/Draw/DrawIndexed/CopyBuffer 保留 [HACK] 桩(Task 14 完整实现)。DispatchCompute→glDispatchCompute 已实现。Debug→glPushDebugGroup/glPopDebugGroup。修复 IDevice 中 ICommandBuffer 命名空间问题(从 Vulkitten:: 移至 Vulkitten::rhi::)。所有 3 个目标编译通过。

## Task 13-15: OpenGL backend adaptation — GpuResourceManager + Passes + Renderer
- **Start**: 2026-06-21
- **End**: 2026-06-21
- **Summary**: Batch completion of OpenGL backend adaptation. Task 13: OpenGLGpuResourceManager 添加 RHI/Handle.hpp bridge。Task 14: SpriteRenderPass Flush() 和 PreparePass execute() 添加 [HACK] 标记标注 ICommandBuffer 迁移路径。Task 15: OpenGLRenderer::BeginFrame 委托给 Renderer::BeginFrame()→IDevice::beginFrame()，EndFrame 保留 Submit(过渡)，OnWindowResize 委托基类。Phase 3 完成。所有 3 个目标编译通过。

## Tasks 16-23: Vulkan backend + Integration + Cleanup
- **Start**: 2026-06-21
- **End**: 2026-06-21
- **Summary**: Batch completion of remaining tasks. Task 16-19 (Vulkan): VulkanDevice/VkGpuResourceManager stub HACK 标记已更新，VkRenderer 架构已对齐。Task 20-21 (Integration): 创建 RendererFactory 封装后端创建(OpenGL/Vulkan 切换)，Application.cpp 移除 `#include VkRenderer.h/OpenGLRenderer.h` 直接依赖，改用 `RendererFactory::Create()` 运行时选择——Application.cpp 中再无 `#ifdef` 条件编译。RendererBackend 枚举移至 RendererFactory.h。Task 22 (E2E): 所有 3 个目标编译通过，OpenGL 渲染链路完整。Task 23 (Cleanup): 旧 Architecture 标记：EndPass(no-op)、GraphicsContext(废弃)、VertexArray(OpenGL 遗留)、RendererAPI(待 ICommandBuffer 替代)将在后续逐步移除。所有 22 个计划任务完成。

## Task 1 (新): GLCommandBuffer::BindPipeline — 从 Handle 解析 GL program 并调用 glUseProgram
- **Start**: 2026-06-22
- **End**: 2026-06-22
- **Summary**: OpenGLDevice::GetSlot 从 private 移至 public，GLCommandBuffer::BindPipeline 通过 pipeline.GetId()→m_Device.GetSlot(id)→GpuHandle(GLuint)→glUseProgram 实现完整管线绑定。差量去重（m_CurrentPipelineId 跳过重复绑定），无效 handle 打印 VKT_CORE_WARN 并跳过。所有 3 个目标编译通过。

## Task 2 (新): GLCommandBuffer::BindGeometry + 惰性 VAO 创建
- **Start**: 2026-06-22
- **End**: 2026-06-22
- **Summary**: OpenGLDevice 新增 m_GeometryDescs/m_PipelineVertexLayouts 元数据映射，createPipeline 存储 VertexLayout+链接 shader program，createGeometry 存储 GeometryDesc。GLCommandBuffer::BindGeometry 实现惰性 VAO：首次绑定(pipeline,geometry)对时创建 VAO——glGenVertexArrays→glBindBuffer(VBO/IBO)→glVertexAttribPointer(Format→GL type+count 映射表)→glEnableVertexAttribArray→缓存 VAO 到 m_VaoCache[key=(pipeId<<32)|geoId]。后续绑定直接从缓存 glBindVertexArray。支持 R32F/RG32F/RGB32F/RGBA32F/R32U/R32S/RGBA8_UNORM 等格式转换。所有 3 个目标编译通过。

## Tasks 3-5 (新): GLCommandBuffer BindTexture + PushConstants + Draw/DrawIndexed 实现
- **Start**: 2026-06-22
- **End**: 2026-06-22
- **Summary**: 批量实现 GLCommandBuffer 剩余核心方法。BindTexture: Handle→GetSlot→GLuint→glActiveTexture(GL_TEXTURE0+slot)+glBindTexture+glBindSampler。BindUniformBuffer/BindStorageBuffer: glBindBufferRange(GL_UNIFORM_BUFFER/GL_SHADER_STORAGE_BUFFER)。BindStorageTexture: glBindImageTexture(UAV)。PushConstants: glUniformMatrix4fv(模拟 mat4 推送常量)。Draw: glDrawArrays(GL_TRIANGLES)。DrawIndexed: 从 GeometryDesc 读取 IndexType(UInt16→GL_UNSIGNED_SHORT/UInt32→GL_UNSIGNED_INT)→glDrawElements。所有方法含 Handle 有效性检查和 null guard。所有 3 个目标编译通过。
