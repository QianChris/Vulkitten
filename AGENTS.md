# AGENTS.md - Vulkitten Engine Development Guide

Guidelines for AI agents contributing to the Vulkitten Engine, a C++17 game engine.

---

## Project Overview
- **Standard**: C++17 | **Build**: CMake 3.16+ with Visual Studio 2022 (x64, Windows)
- **Architecture**: DLL (`Vulkitten`) + Executable (`SandBox`) + Editor (`VulkittenEditor`)
- **Renderer**: OpenGL 4 (GLAD) via abstract `RendererAPI`; RenderGraph pipeline (in-progress); Vulkan planned
- **ECS**: EnTT-based with `Scene` owning `entt::registry`, `Entity` wrappers, `System` interface
- **Editor**: Docked ImGui panels, ImGuizmo, undo/redo command system, entity picking
- **Key Dependencies**: spdlog, glm, glfw, glad, imgui, entt, yaml-cpp, imguizmo, imnodes, tinygltf

---

## Build, Lint, & Test Commands
### Build
```bash
# Quick debug build (x64)
build.bat
# Manual:
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Debug

# Build specific target
cmake --build build --target Vulkitten --config Debug
cmake --build build --target SandBox --config Release

# Clean and rebuild
cmake --build build --target clean --config Debug && cmake --build build --config Debug
```
Binaries: `bin/[Config]-x64/[ProjectName]/` (e.g., `bin/Debug-x64/SandBox/SandBox.exe`)

### Lint
Use vendored `.clang-format` (4 spaces, 100-char limit, CRLF endings).

### Test
No test framework configured. Verify changes via successful builds and manual execution of `SandBox.exe`.

---

## Code Style Guidelines
### Formatting & Structure
- **Braces**: K&R style, opening brace on new line
- **Access Modifiers**: No extra indent; members indented once

### Include Order
1. Corresponding header (for `.cpp`) | 2. Precompiled header (`vktpch.h`)
3. System headers (`<vector>`, `<string>`) | 4. Third-party headers (`<spdlog/spdlog.h>`, etc.)
5. Project headers (`"Vulkitten/Core.h"`)

### Naming Conventions
| Element | Convention | Example |
|---------|------------|---------|
| Namespace/Class/Struct | PascalCase | `Vulkitten` |
| Function | PascalCase | `BeginSession` |
| Variable | camelCase | `deltaTime` |
| Instance Member | `m_` + camelCase | `m_Width` |
| Static Member | `s_` + camelCase | `s_GLFWInitialized` |
| Constant | kPascalCase / UPPER_SNAKE | `kMaxEntities` |
| Enum/Enum Value | PascalCase | `EventType::KeyPressed` |
| Macro | UPPER_SNAKE_CASE | `VKT_API` |

### Error Handling
- **Assertions**: `VKT_ASSERT(cond, "msg")` (client) or `VKT_CORE_ASSERT` (engine)
- **Logging**: Use `VKT_CORE_*` (engine) / `VKT_*` (client) macros; use `.string()` for paths
- **DLL Exports**: Mark with `VKT_API`: `class VKT_API Application { ... };`

---

## Key Subsystems
- **Platform (Window & Surface)**: Two-tier window interface — `Window` (application layer: events, VSync, OnUpdate) and `IWindow` (backend layer: `GetSurfaceDesc()`, `GetSurface() → ISurface*`). `ISurface` abstracts the platform drawable (wraps GLFWwindow / HWND) and exposes `GetNativeHandle()` for Vulkan surface creation. `WindowsWindow` implements both `Window` and `IWindow`; `WindowsSurface` implements `ISurface`.
- **ClassFactory (DI Root)**: Meyer's singleton, engine's centralized DI container. `GetInstance<T>()` for singleton creation/retrieval, `RegisterInstance<T>(T*)` for externally-created objects. `GetInterface<I>()` / `RegisterInterface<I, Impl>()` for interface-implementation mapping (e.g., Device backend). All new engine singletons (Engine, RenderContext, GraphicContext) are created through ClassFactory.
- **Engine (Core)**: Singleton created via `ClassFactory::GetInstance<Engine>()`. Owns FileSystem instance, manages Input lifecycle (keeps static `Input::IsKeyPressed()` access), provides `GetLogger()` → `Log::Get()`. Empty EventQueue/ThreadPool stubs for future use. `Engine::Init()`/`Shutdown()` lifecycle methods (not yet wired into main — Task 12).
- **Event System**: Inherit from `Event`, use `EVENT_CLASS_TYPE`/`EVENT_CLASS_CATEGORY` macros, dispatch via `EventDispatcher`
- **Layers**: `PushLayer` (in-order) or `PushOverlay` (always on top); Application runs OnUpdate then OnImguiRender for all layers
- **ECS**: `Scene` owns `entt::registry`. `Entity` wraps `entt::entity` with `AddComponent`/`GetComponent`. Components are POD structs in `Components.h`. Systems implement `System::OnUpdate(Scene&, Timestep, bool)`.
- **Renderer (see [ARCHITECTURE.md](Vulkitten/ARCHITECTURE.md#4-分层渲染器架构))**:
  - `IRenderer` — sole backend interface (Init/Shutdown/BeginFrame/EndFrame/Execute/OnWindowResize). Platform & scene layers only interact with this interface.
  - `RendererConfig` — packs IDevice*/RenderGraph*/IGpuResourceManager* for IRenderer creation.
  - `ICommandBuffer` (`RHI/ICommandBuffer.hpp`) — pure virtual command recording interface: Begin/End lifecycle, Barrier (global + texture), BeginRenderPass/EndRenderPass, BindPipeline/BindGeometry, BindUniformBuffer/BindStorageBuffer/BindTexture/BindStorageTexture (slot-based), PushConstants, Draw/DrawIndexed/DispatchCompute, CopyBuffer/CopyBufferToTexture/CopyTextureToBuffer, BeginDebugLabel/EndDebugLabel/InsertDebugMarker. Slots used at bind time must be subset of PipelineDesc's TextureSlots/BufferSlots declarations. Vulkan: true recording; OpenGL: immediate execution with state caching.
- `IDevice` — abstract GPU device interface: Init/Shutdown + beginFrame/endFrame frame lifecycle + createCommandBuffer + 8 resource creation methods (createBuffer→BufferHandle, createTexture→TextureHandle, createShader→ShaderHandle, createPipeline→PipelineHandle, createGeometry→GeometryHandle, createSampler→SamplerHandle, createRenderPass→RenderPassHandle, createFramebuffer→FramebufferHandle) + onResize/waitIdle/getNativeDevice. Stub implementations in OpenGLDevice and VulkanDevice (full impl in Tasks 10/15). `ShaderBytecode` struct (data ptr + size + entry point) defined alongside. Descriptor structs (BufferDesc/TextureDesc/RenderPassDesc/FramebufferDesc/etc.) in `RHI/RHIResourceDescs.hpp`. Legacy `Submit(FrameContext&)` retained for backward compat.
  - `IGpuResourceManager` — GPU resource management interface (CreateTexture/CreateBuffer/CreateShader/CreatePipeline/CreateGeometry → uint64_t handles). All GPU resources go through this. `GpuResourceManager` implements it.
  - `GpuResourceManager` — centralized VRAM resource manager implementing IGpuResourceManager. Handle encoding (index+generation), deferred allocation, reference tracking, GC.
  - `ShaderManager` — shader loading + #include preprocessing. Constructor injects `FileSystem&`.
  - `GltfLoader` — glTF 2.0 loader wrapping tinygltf.
  - `RendererAPI` — abstract base for low-level draw commands (Init, Clear, DrawIndexed). Being superseded by IRenderer + per-pass RenderContext.
  - `FrameContext` — transient per-frame struct: FrameIndex + SwapchainIndex + void* Internal (backend-private). Owned by IDevice::beginFrame()/endFrame(). Backend sync objects (Fence/Semaphore/CommandPool) live inside Internal or backend-private fields, invisible to upper layers.
  - `RenderContext` (per-pass) — translates RenderCommands to API drawcalls. Holds FrameContext& + IRenderer&. Uses PipelineHandle/GeometryHandle for state caching.
  - `RendererSubsystem` — rendering subsystem singleton (was RenderContext). Owns Renderer (the IRenderer impl), RenderGraph, RenderUtils, ShaderLibrary.
  - `Renderer` — scene-level IRenderer implementation (owns RenderGraph instance, registers Passes).
  - `RenderGraph` — command-based pass system with `Execute()` iterating passes and dispatching commands.
  - `RenderSystem` — ECS System that creates `RenderCommand`s from components. Receives RenderGraph via SceneContext injection.
- **Vulkan Backend** (`Renderer/Backend/Vulkan/`): Skeleton implementation with all IRenderer/IDevice/IGpuResourceManager interfaces satisfied. `VulkanInstance` wraps VkInstance + validation; `VulkanDevice` handles physical/logical device creation; `VkSwapchain` manages surface + swapchain; `VkFrameContext`/`VkRenderContext` per-frame/per-pass state; `VkGpuResourceManager` handle-based resource management; `VkShader`/`VkPipeline` pipeline creation stubs; `VkRenderer` top-level IRenderer impl. Select backend via `Application::SetBackend(RendererBackend::Vulkan)` before construction.
- **RHI (Rendering Hardware Interface)** (`RHI/`): New abstraction layer under `Vulkitten::rhi` namespace providing backend-agnostic rendering primitives. Foundation complete (Phase 1 Tasks 1-4):
  - `RHI/Core/Types.hpp` — Extent2D/3D, Viewport, ClearValue, ShaderStage, BufferUsage/TextureUsage/MemoryProperty, PipelineStage/AccessFlags/ImageLayout, FilterMode/WrapMode, LoadOp/StoreOp
  - `RHI/Core/Format.hpp` — `Format` enum (R8_UNORM..BC7_RGBA_UNORM, D16_UNORM..D32_FLOAT_S8_UINT) with `FormatByteSize`/`FormatComponentCount`/`IsDepthFormat`/`IsStencilFormat`
  - `RHI/Handle.hpp` — `Handle<Tag>` (uint32_t id + generation) with 8 Tag types for compile-time type safety
  - `RHI/RHIPipelineDesc.hpp` — `PipelineDesc` (VertexAttribute[] + RasterState + DepthStencilState + BlendState[] + `TextureSlots`[slot/Type/Stages] + `BufferSlots`[slot/Type/Stages/Size]), `TextureSlot`/`BufferSlot` structs declare binding interface at pipeline creation time, `GeometryDesc` (NO vertex format), `SamplerDesc`
  All RHI types are pure C++17 — no Vulkan or OpenGL types in RHI headers. Phase 2 (Tasks 5-9) will build the abstract interfaces.
- **Entry Point**: Implement `Vulkitten::CreateApplication()` returning `Application*`

---

## RenderGraph Pipeline (In Progress)

- **`RenderGraph`** owns `vector<RenderPass> m_Passes` and `vector<RenderCommand> m_FrameCommands`; exposes `GetPassCount()` / `GetPassName(index)` for external querying
- **`RenderCommand`** is `std::variant<DrawQuadCommand, ClearCommand>` (extensible — DrawMesh, DrawParticle, DrawFullscreen are reserved but commented out)
- **`RenderPass`** has inputs/outputs (`PassResourceUsage` with `AccessFlags`/`pipelineStages`/`imageLayout`), `writesToSwapchain` flag, and `onExecute` callback
- **`ResourcePool`** provides type-erased GPU resource management with generation-counted handles (`Handle<T>`), free-list allocation, and per-frame deferred deletion (`MAX_FRAMES_IN_FLIGHT`)
- **`RenderGraphResource`** describes a resource by name, type (Texture2D/TextureCube/Buffer/SwapchainImage), and descriptor union
- **Current state**: `Execute()` dispatches commands to registered passes (PreparePass→GpuParticlePass→SpriteRenderPass→EndPass). `Renderer2D` draws within SpriteRenderPass. `Scene::OnUpdate` always routes through RenderGraph.

---

## GPU Particles (`Vulkitten/Scene/GpuParticle/`)

- Uses **direct OpenGL compute shaders** — does NOT go through RendererAPI or RenderGraph
- **Double-buffered SSBO** pairs for particle data and indirect args
- **4 compute passes per frame**: SimArg (reset args) → Sim (indirect dispatch, update existing) → Emit (direct dispatch, spawn new) → RenderArg (write indirect draw args)
- Renders as `GL_POINTS` via `glDrawArraysIndirect`
- Managed per-emitter by `GpuEmitterManager` (references engine ShaderLibrary from RenderContext)
- Shaders loaded from `engine://shaders/` and `engine://computeshaders/` (Vulkitten/assets/)
- **Architectural note**: This is technical debt — future work should integrate compute into RenderGraph as a pass type

---

## Editor Architecture (VulkittenEditor)

### Context and Signals
- `EditorContext` (in `EditorContext.h`) is the single shared state — scene, selected/hovered entity, editor camera, SignalBus, CommandSystem pointer
- `SignalBus` provides type-safe publish/subscribe between panels without direct coupling — subscribe with `Subscribe<T>(handler)`, publish with `Publish<T>(event)`
- Editor events: `EntitySelected`, `EntityHovered`, `EntityDestroyed`, `ComponentModified`, `SceneModified`, `ViewportResized`, `RequestNewScene`, `RequestOpenScene`, `RequestSaveScene`

### Undo/Redo (CommandSystem)
- `EditorCommand` base: `Execute()`, `Undo()`, `GetName()`
- `CommandSystem` maintains history (max 50), supports `CanUndo()`/`CanRedo()`
- Concrete commands: `DestroyEntityCommand`, `SetTransformCommand`
- Commands created at gizmo release (ViewportPanel) or property edit completion (PropertyPanel)

### Panels
All panels in `VulkittenEditor/src/Panel/` implement the `IPanel` interface:
```cpp
class IPanel {
public:
    virtual void OnAttach(EditorContext* context);
    virtual void OnDetach();
    virtual void OnUpdate(Timestep ts);
    virtual void OnUIRender() = 0;
    virtual void OnEvent(Event& event);
    bool IsOpen;
};
```

| Panel | Purpose |
|-------|---------|
| `ViewportPanel` | Framebuffer rendering, ImGuizmo (T/R/Y), entity mouse picking (RED_INTEGER attachment) |
| `SceneHierarchyPanel` | Entity tree with add/delete context menus |
| `PropertyPanel` | Edit selected entity's Tag, Transform, SpriteRenderer, Camera, NativeScript components |
| `PerformancePanel` | FPS, frame time, Renderer2D stats (DrawCalls, Quads, Vertices, TextureCount) |
| `ResourcePanel` | Visualize texture resources with usage counts |

### Panel Initialization
In `EditorLayer::OnAttach()`:
```cpp
m_ViewportPanel->OnAttach(&m_Context);
m_SceneHierarchyPanel->OnAttach(&m_Context);
// etc.
```
UIRender order (within dockspace): Viewport → SceneHierarchy → Property → Resource → Performance

---

## Texture & Serialization
- **SpriteRendererComponent**: Stores `Texture` (Ref) and `TexturePath` (string) for serialization
- **YAML**: `TexturePath: "sandbox://assets/textures/Checkerboard.png"`
- **Deserialization**: `sprite.Texture = Texture2D::Create(sprite.TexturePath);`
- **FileSystem**: Instance-based, owned by Engine. All access via `Engine::Get().GetFileSystem().RegisterPath/Resolve/Exists()`. No static methods remain.

---

## Template & Memory
- Template definitions must stay in headers
- Use `Ref<T>` for renderer resources
- Use RAII (`std::lock_guard`); avoid manual memory management

---

## Vendor Dependencies

Git submodules (in `Vulkitten/vendor/`):

| Library | Purpose |
|---------|---------|
| `spdlog` | Logging framework |
| `glm` | Math library (vec/mat/quat) |
| `glfw` | Window creation + input |
| `imgui` | Immediate-mode GUI (Dear ImGui) |
| `entt` | ECS library (header-only, C++17) |
| `yaml-cpp` | YAML serialization |
| `ImGuizmo` | 3D gizmo for ImGui (Translate/Rotate/Scale) |
| `imnodes` | Node editor for ImGui (future use) |
| `tinygltf` | glTF 2.0 model loader (header-only) |

Manual (header-only, no submodule):
- `nlohmann/json` — JSON parsing in `vendor/nlohmann/include/`
- `stb_image` — Image loading in `vendor/stb_image/`
- `glad` — OpenGL function loader in `vendor/Glad/`

---

## Notes
- Only Windows (x64) + OpenGL supported; Vulkan planned
- Third-party libs vendored in `Vulkitten/vendor/`
- All `.cpp` files must include `vktpch.h` first
- Use `VKT_PROFILE_*` macros for profiling
- Commit format: `type: short description` (e.g., `feat: add 2D renderer`)