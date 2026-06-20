# TASK_LOG.md — Vulkitten Engine Task Execution Log

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
