# TASK_LOG.md — Vulkitten Engine Task Execution Log

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
