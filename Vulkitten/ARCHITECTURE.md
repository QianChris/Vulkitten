# ARCHITECTURE.md — Vulkitten 引擎设计与结构

## 1. 项目解剖

```
VulkittenEngine/
├── Vulkitten/              # 引擎核心 DLL
│   ├── src/
│   │   ├── Vulkitten.h     # 统一对外头文件
│   │   ├── vktpch.h/cpp    # 预编译头
│   │   └── Vulkitten/
│   │       ├── Core/       # 应用框架、Window/IWindow、日志
│   │       ├── Events/     # 事件系统
│   │       ├── Renderer/   # 渲染器抽象 (IRenderer/IFrameContext) + RenderGraph
│   │       │   └── Backend/ # OpenGL/Vulkan 后端实现
│   │       ├── Window/     # 平台窗口实现
│   │       │   └── Platform/Windows/ # GLFW 窗口 + 输入
│   │       ├── Scene/      # ECS（Scene, Entity, Components, Systems）
│   │       ├── ImGui/      # ImGui 集成层
│   │       ├── Perf/       # 性能分析（Instrumentor, Timer）
│   │       └── Utils/      # 工具（FileDialogs, YAMLConversions）
│   ├── assets/             # 引擎 Shader 资产（shaders/, computeshaders/）
│   └── vendor/             # 8 个 git submodule + 手动依赖
├── Sandbox/                # 测试应用 (Sandbox.exe)
├── VulkittenEditor/        # 编辑器应用 (VulkittenEditor.exe)
└── tools/                  # 文件树生成器、Shader 编译器
```

**构建系统：** CMake 3.16+, C++17, Visual Studio 2022 (x64, Windows)
- 根 `CMakeLists.txt` 定义三项目构建顺序：Vulkitten → Sandbox → VulkittenEditor
- 输出：`bin/{Config}-x64/{ProjectName}/`
- 中间文件：`bin-int/{Config}-x64/{ProjectName}/`
- Vulkitten 为 DLL（`VULKITTEN_BUILD_DLL`），Sandbox/Editor 链接 Vulkitten 并通过 post-build 复制 DLL

---

## 2. 核心应用框架

### Engine (引擎核心单例)

```cpp
class Engine {
public:
    void Init();                       // 初始化子系统
    void Shutdown();                   // 清理子系统

    FileSystem& GetFileSystem();       // 虚拟文件系统实例
    static Log& GetLogger();           // Log 单例引用 (VKT_CORE_* 宏不变)
    static Engine& Get();              // ClassFactory::GetInstance<Engine>()

    struct EventQueue {};              // 空壳：预留 DeferredEvent 容器
    struct ThreadPool {};              // 空壳：预留工作线程池
};
```

`Engine` 通过 `ClassFactory::GetInstance<Engine>()` 创建（Meyer's singleton）。它持有所有引擎子系统：`FileSystem`(实例)、`Input`(生命周期管理)、`Log`(引用)。`Engine::Init()` 负责子系统初始化，`Engine::Shutdown()` 负责清理。当前 Engine 尚未接入 Application 主循环（Task 12 完成）。`EventQueue` 和 `ThreadPool` 为空壳 struct，待后续任务实现。

### 启动流程

```
main() [EntryPoint.h]
  └─ Vulkitten::CreateApplication()    // 由客户端实现，返回 Sandbox 或 EditorApp
       └─ Application::Application()   // 构造函数
            ├─ Window::Create()        // → WindowsWindow (GLFW)
            ├─ OpenGLContext::Init()   // 加载 GLAD
            ├─ Renderer::Init()        // 初始化 Renderer2D + RenderGraph
            └─ PushOverlay(ImGuiLayer) // ImGui 始终在最上层
```

### 主循环

```
Application::Run()
  while (m_Running):
    timestep = 计算 deltaTime
    for each layer in m_LayerStack:      // 正序
      layer->OnUpdate(timestep)          // 游戏逻辑 + 渲染
    m_ImGuiLayer->Begin()                // ImGui 新帧
    for each layer in m_LayerStack:
      layer->OnImguiRender()             // ImGui UI
    m_ImGuiLayer->End()                  // ImGui 渲染 + 绘制
    m_Window->OnUpdate()                 // glfwPollEvents + SwapBuffers
```

### Layer 与 LayerStack

```
LayerStack
├── [0..m_LayerInsertIndex)  ← 普通 Layer (PushLayer 插入位置)
└── [m_LayerInsertIndex..n)  ← Overlay (PushOverlay 添加到末尾)
```

- 事件分发：**倒序遍历**（Overlay 先处理），Layer 标记 `Handled` 后停止传播
- 示例：EditorLayer 为普通 Layer，ImGuiLayer 为 Overlay

### 事件系统

```
Event (抽象基类)
├── EventType (枚举：KeyPress, MouseMove, WindowResize 等)
├── EventCategory (位掩码：Application, Input, Keyboard, Mouse, MouseButton)
└── Handled (bool，标记事件已被消费)

EventDispatcher<T>
  └─ Dispatch<T>(event, std::function<bool(T&)>)
       └─ 通过 T::GetStaticType() 匹配并调用回调
```

宏：`EVENT_CLASS_TYPE(type)` / `EVENT_CLASS_CATEGORY(cat)`
具体事件类：`WindowResizeEvent`, `KeyPressedEvent`, `MouseMovedEvent` 等（见 Events/ 目录）

### DLL 导出

```cpp
// Core.h
#define VKT_API __declspec(dllexport/dllimport)

template<typename T> using Ref = std::shared_ptr<T>;
template<typename T> using Scope = std::unique_ptr<T>;

// 类必须是完整定义（用于 shared_ptr 的 delete）
#define CreateRef<T>(args) std::make_shared<T>(args)
#define CreateScope<T>(args) std::make_unique<T>(args)
```

### ClassFactory (唯一单例 + DI 容器)

```cpp
class ClassFactory {
public:
    static ClassFactory& Get();       // Meyer's Singleton

    UUID GenerateUUID();              // 统一 UUID 生成

    // ---- DI: 实例管理 ----
    template<typename T>
    static T& CreateInstance();       // Meyer's singleton lazy-create（每类型一个静态局部变量）

    template<typename T>
    static void RegisterInstance(T*); // 注册外部创建的单例，优先于 CreateInstance

    template<typename T>
    static T& GetInstance();          // 查找已注册实例 → 否则 CreateInstance + 缓存

    // ---- DI: 接口管理 ----
    template<typename I, typename Impl>
    static void RegisterInterface();  // 注册接口实现类型（通过 GetInstance<Impl> 创建）

    template<typename I>
    static void RegisterInterface(I* impl); // 注册已有实例作为接口实现

    template<typename I>
    static I& GetInterface();         // 获取接口实现（未注册时 VKT_CORE_ASSERT 触发）

    // ---- Legacy 访问器（将逐步迁移到 GetInstance<T>()） ----
    Application& GetApplication();
    Window& GetWindow();
    RenderGraph* GetRenderGraph();
};
```

`ClassFactory` 是引擎的 DI 根容器。所有后继单例（Engine、RenderContext、GraphicContext）通过它统一创建和管理。内部使用 `std::type_index → void*` 映射存储注册实例，`std::type_index → std::function<void*()>` 映射存储接口工厂。位于 `Vulkitten/Core/ClassFactory.h`，已加入 `Vulkitten.h` 统一头文件。

---

## 3. ECS 架构

### Entity

```cpp
class Entity {
    entt::entity m_EntityHandle;  // EnTT 实体 ID
    Scene* m_Scene;               // 反向指针

    template<typename T> T& AddComponent(Args&&... args);
    template<typename T> T& GetComponent();
    template<typename T> bool HasComponent();
    template<typename T> void RemoveComponent();

    operator uint32_t() const;    // 转换为 entity ID
};
```

### Components（定义在 `Vulkitten/Scene/Components.h`）

| Component | 字段 | 说明 |
|-----------|------|------|
| `TagComponent` | `std::string Tag` | 实体名称 |
| `TransformComponent` | `vec3 Position, Rotation, Scale` + `mat4 Transform` (缓存, 脏标记) | 空间变换 |
| `SpriteRendererComponent` | `vec4 Color, Ref<Texture2D> Texture, string TexturePath, float TilingFactor` | 2D 精灵渲染 |
| `CameraComponent` | `SceneCamera Camera, bool Primary, bool FixedAspectRatio` | 相机 |
| `NativeScriptComponent` | `string ClassName, ScriptableEntity* Instance, Bind<T>()` | C++ 脚本组件 |
| `GpuEmitterComponent` | `string TexturePath, uint32_t MaxParticles` | GPU 粒子发射器 |

### System 接口

```cpp
class System {
public:
    virtual ~System() = default;
    // shouldRender: 引擎告知是否需要渲染
    // 返回 true 表示本 System 已提交渲染命令，需要渲染
    virtual bool OnUpdate(Scene& scene, Timestep timestep, bool shouldRender) = 0;
};
```

- Scene 持有 `std::vector<Scope<System>> m_Systems`
- 通过 `Scene::AddSystem(CreateScope<MySystem>())` 注册
- 执行顺序：按添加顺序

### Scene 生命周期

```
Scene::OnUpdate(Timestep)
  ├─ TickScripts()              // 调用 NativeScriptComponent 的 OnUpdate
  ├─ for each System:
  │    system->OnUpdate(scene, ts, shouldRender)
  │    if (returned true) → shouldRender = true
  └─ if (shouldRender):
       RenderScene(*camera)     // 当前实际渲染路径
         ├─ GPU 粒子：Update + Render (原始 GL 调用)
         └─ Renderer2D::BeginScene→DrawQuad→EndScene
```

> **注意**：`OnUpdateRuntime()` 为空桩，待实现运行时（Play 模式）的逻辑。

---

## 4. 分层渲染器架构

渲染完全通过 RenderGraph 进行，旧版直接 Renderer2D 路径已移除。Scene 通过 SceneContext 注入获取 RenderGraph。

```
Application::Run()
  ├─ IRenderer::BeginFrame()         → 创建 FrameContext
  ├─ SceneContext(IRenderer&, RenderGraph&) → 注入到 Layer::OnUpdate
  │
  ├─ Layer::OnUpdate(timestep, sceneCtx)
  │    └─ Scene::OnUpdate(ts, ctx)
  │         └─ RenderSystem::OnUpdate(..., ctx)
  │              └─ ctx.GetRenderGraph().AddCommand(ClearCommand/DrawQuadCommand)
  │         └─ ctx.GetRenderGraph().SetPerFrameData(...)
  │
  ├─ ImGui 绘制
  ├─ IRenderer::Execute()           → RenderGraph::Execute()
  │    └─ PreparePass → GpuParticlePass → SpriteRenderPass → EndPass(no-op)
  └─ IRenderer::EndFrame()          → SwapBuffers + GPU 同步
```

SwapBuffers 已从 EndPass 迁移至 IRenderer::EndFrame()。
```
```
### 六层抽象（自上而下）

| 层 | 类/文件 | 职责 | 状态 |
|----|---------|------|------|
| **7. SceneContext** | `Scene/SceneContext` | 注入 IRenderer& + RenderGraph&，解耦 Scene 与全局单例 | ✅ 新增 |
| **6. RenderSystem** | `Scene/Systems/RenderSystem` | ECS→RenderCommand 桥接，遍历实体生成 DrawQuadCommand + ClearCommand，通过 SceneContext 获取 RenderGraph | ✅ 已注入化 |
| **5. RenderGraph** | `Renderer/RenderGraph/RenderGraph` | 命令式延迟渲染管线，管理 Passes 和 FrameCommands | ✅ Execute() 已实现 |
| **4. IRenderer** | `Renderer/IRenderer` | 后端统一接口：BeginFrame→Execute→EndFrame 生命周期 | ✅ Renderer 实现 |
| **3. Renderer** | `Renderer/Renderer` | IRenderer 实现，持有 RenderGraph 实例，注册 Passes，管理 FrameContext | ✅ 接入主循环 |
| **2. Renderer2D / SpriteRenderPass** | `Renderer/Passes/SpriteRenderPass` | 即时模式批处理四边形渲染器 (10000 四边形/32 纹理槽) | ✅ 被 SpriteRenderPass 内部使用 |
```

### 六层抽象（自上而下）

| 层 | 类/文件 | 职责 | 状态 |
|----|---------|------|------|
| **6. RenderSystem** | `Scene/Systems/RenderSystem` | ECS→RenderCommand 桥接，遍历实体生成 DrawQuadCommand + ClearCommand | ✅ 可用 |
| **5. RenderGraph** | `Renderer/RenderGraph/RenderGraph` | 命令式延迟渲染管线，管理 Passes 和 FrameCommands | ✅ Execute() 已实现，按 Pass 名称过滤命令 |
| **4. Renderer** | `Renderer/Renderer` | 场景级抽象，持有 RenderGraph 实例，注册 Passes，提供 Render() | ✅ 已接入主循环 |
| **3. Renderer2D** | `Renderer/Renderer2D` | 即时模式批处理四边形渲染器 (10000 四边形/32 纹理槽) | ✅ 被 SpriteRenderPass 内部使用 |
| **2. Legacy::RenderCommand** | `Renderer/RenderCommand` | 静态代理类包装 RendererAPI* 单例，提供内联 Clear/Draw 方法 | ✅ 被 PreparePass 内部使用 |
| **1. RendererAPI** | `Renderer/RendererAPI` | 最低层 GPU 抽象：Init, Clear, DrawIndexed 等纯虚接口 | ✅ OpenGL 实现 |
| **0. Device** | `Renderer/Device` | GPU 设备抽象：Init/Shutdown。OpenGL 中 GL Context 即为 Device；Vulkan 中持有 VkDevice/VkPhysicalDevice | ✅ 抽象类 + OpenGLDevice 空壳占位 |
| **GpuResourceManager** | `Renderer/GpuResourceManager` | 统一显存资源管理器：uint64_t 句柄 (index+generation)，CreateTexture/CreateBuffer 返回句柄，支持延时分配 (记录 desc，首次 Get 时分配 GPU 资源) | ✅ 骨架 (句柄分配/查找/延时创建占位) |
| **ShaderManager** | `Renderer/ShaderManager` | Shader 加载 + #include 预处理：构造注入 FileSystem&，LoadShader(virtualPath) 解析路径→读取→递归展开 #include→返回 uint64_t 句柄 | ✅ 新增 (未替换旧 OpenGLShader 路径) |
| **GltfLoader** | `Renderer/Gltf/GltfLoader` | glTF 2.0 模型加载器（基于 tinygltf）：构造注入 FileSystem&，Load(virtualPath) 解析 GLB/glTF→提取顶点/索引数据→返回 GltfMeshData 向量。支持位置/法线/UV | ✅ 新增 |

### 工厂模式（抽象 → 平台）

所有 GPU 资源通过静态 `Create()` 工厂方法创建，内部 switch `RendererAPI::GetAPI()`：

```cpp
// 示例：RendererAPI.h
static API GetAPI() { return API::OpenGL; }  // 编译时确定

// 示例：Texture.h
static Ref<Texture2D> Create(uint32_t width, uint32_t height);
// Texture.cpp → switch(API) → new OpenGLTexture2D(...)
```

---

## 5. RenderGraph 深度解析

> **状态**：核心执行流程已完成。Execute() 按 Pass 名称过滤命令 → 调用 onExecute 回调 → 渲染完成。

### 核心数据结构

```
RenderGraph
├── vector<RenderPass> m_Passes         // 已注册的 Pass (持久)
├── vector<RenderCommand> m_FrameCommands  // 帧内所有渲染命令 (每帧清空)
├── vector<RenderGraphResource> m_Resources  // 资源描述符
├── void* m_BackendContext              // 后端上下文 (EndPass 用于 SwapBuffers)
├── glm::mat4 m_ViewProjection          // 当前帧的 VP 矩阵
├── Camera* m_SceneCamera               // 当前场景相机
├── AddPass(pass)                       // 注册 Pass (在 Renderer::Init 中调用)
├── AddCommand(cmd)                     // 收集命令（由 RenderSystem 调用）
├── Execute()                           // ✅ 遍历 Passes，按名过滤命令，执行 onExecute
├── GetPassCount()                      // 🔍 返回已注册 Pass 数量
├── GetPassName(index)                  // 🔍 返回指定索引 Pass 的名称
└── ClearFrameCommands()                // 每帧清空命令
```

### 已注册 Passes

| Pass | 过滤命令类型 | onExecute 行为 |
|------|-------------|---------------|
| **PreparePass** | `ClearCommand` | 若 graph 配置了 Framebuffer 则绑定额外 Framebuffer；调用 `Legacy::RenderCommand::SetClearColor` + `Clear`；解绑 |
| **GpuParticlePass** | (无, 直接操作 ECS) | 从 graph 获取 Scene + Camera；遍历 `GpuEmitterComponent` 实体；调用 `Update()` + `Render()` |
| **SpriteRenderPass** | `DrawQuadCommand` | 若 graph 配置了 Framebuffer 则绑定额外 Framebuffer；从 graph 获取 Camera，调用 `Renderer2D::BeginScene` → `DrawQuad` × N → `EndScene`；解绑 |
| **EndPass** | (无) | 将 `backendContext` 转为 `GraphicsContext*`，调用 `SwapBuffers`（始终作用于默认 Backbuffer） |

### 执行流程

```
Scene::OnUpdate
  └─ RenderSystem::OnUpdate → 添加 ClearCommand + DrawQuadCommand × N
  └─ Scene 设置 Camera + VP 到 RenderGraph
  └─ 返回 (不调用 RenderScene)

Application::Run
  └─ ... ImGui 渲染 ...
  └─ Renderer::Render() → m_graph->Execute()
       ├─ PreparePass::onExecute → ClearCommand → glClear
       ├─ SpriteRenderPass::onExecute → Renderer2D 批量绘制
       └─ EndPass::onExecute → SwapBuffers
```

### RenderPass

```cpp
class RenderPass {
    const char* m_Name;                   // Pass 名称（调试）
    uint32_t m_Index;                     // Pass 索引
    vector<PassResourceUsage> m_Inputs;   // 输入资源声明
    vector<PassResourceUsage> m_Outputs;  // 输出资源声明
    bool m_WritesToSwapchain;             // 是否写入交换链
    function<void(ResourcePool&, span<RenderCommand>, void*)> m_OnExecute;
    //  ↑ ExecuteFunc: 接收资源池、过滤后的命令、后端上下文指针
};
```

- 链式 API：`RenderPass::Read(resource)` / `Write(resource)` / `SetExecute(fn)`
- Pass 通过 `AddPass()` 注册到 RenderGraph

### RenderCommand

```cpp
using RenderCommand = std::variant<
    DrawQuadCommand,     // 颜色、纹理、平铺因子、变换矩阵
    ClearCommand         // 颜色/深度/模板清除标志
    // 注释掉但预留：DrawMeshCommand, DrawParticleCommand, DrawFullscreenCommand
>;
```

### RenderGraphResource

```cpp
struct RenderGraphResource {
    string m_Name;                       // 资源名称
    ResourceType m_Type;                 // Texture2D | TextureCube | Buffer | SwapchainImage
    union { TextureDesc textureDesc; BufferDesc bufferDesc; };
    uint32_t m_PhysicalIndex;            // 编译后的物理索引
    bool m_Imported;                     // 是否为导入资源（如 Swapchain）
};
```

### ResourcePool & ResourceHandle

```
ResourcePool (类型擦除的 GPU 资源池)
├── vector<ResourceSlot> m_Slots        // 资源槽数组
│   └── ResourceSlot:
│       union { uint64_t buffer; uint64_t texture; uint64_t framebuffer; }
│       uint16_t generation;            // 代数计数器（防悬空指针）
│       bool alive; uint32_t size; uint32_t lastUsedFrame;
├── queue<uint32_t> m_FreeIndices       // 空闲槽索引队列
├── CreateBuffer/CreateTexture/CreateFramebuffer → Handle<T>
├── Get(Handle)/Destroy(Handle)
└── per-frame deferred deletion (MAX_FRAMES_IN_FLIGHT)

ResourceHandle<T> = { uint32_t index, uint16_t generation }
```

### 当前缺口（待实现）

1. Pass 的资源依赖分析和 barrier 插入（目前仅在 PassResourceUsage 中声明，未被 Execute 使用）
2. `ResourcePool` 的 GPU 资源分配/转换（数据结构和 Handle 已就绪，Execute 中未集成）
3. GPU 粒子系统接入 RenderGraph（当前仍绕过所有抽象）
4. EditorLayer 的 Framebuffer 配置迁移到 Pass 配置（Task 2）

---

## 6. GPU 粒子系统

> **注意**：该系统绕过所有渲染器抽象，直接使用 OpenGL 调用。这是架构债，需在后续 Phase 中解决。

### 架构

```
GpuEmitterManager (Scene 持有, GpuParticle/GpuParticle.h)
├── ShaderLibrary引用 (从 RenderContext 获取，引擎预加载)
└── unordered_map<TexturePath, GpuEmitterInstance>
       └── GpuEmitterInstance:
            ├── VAO (Empty VAO, 使用 SSBO 作为顶点数据)
            ├── SSBO ping-pong 对: ParticleBuffer [count * sizeof(Particle)]
            ├── SSBO: DeadListBuffer
            ├── SSBO ping-pong 对: ParticleCountBuffer [sizeof(uint32_t)]
            ├── SSBO: IndirectArgs [sizeof(DispatchIndirectCommand)]
            ├── SSBO: IndirectDrawArgs [sizeof(DrawElementsIndirectCommand)]
            └── UBO: EmitterData (发射器参数)
```

### 每帧计算管线 (4 Pass)

```
1. SimArg      → glDispatchCompute(1,1,1)   清空间接参数
2. Sim         → glDispatchComputeIndirect   更新已有粒子（间接 dispatch）
3. Emit        → glDispatchCompute(N)        生成新粒子（直接 dispatch）
4. RenderArg   → glDispatchCompute(1,1,1)   写入间接绘制参数
   [Ping-Pong Swap]

Render: glBindVertexArray→glDrawArraysIndirect(GL_POINTS)
```

所有 shader 位于 `Sandbox/assets/computeshaders/`，通过 `FileSystem` 的 `sandbox://` 路径加载。

---

## 7. 编辑器架构

### EditorApp → EditorLayer

```
EditorApp::EditorApp()
  ├─ FileSystem::RegisterPath("../../VulkittenEditor", "editor")
  ├─ FileSystem::RegisterPath("../../VulkittenEditor/assets/icons", "editorIcons")
  ├─ FileSystem::RegisterPath("../../Sandbox", "sandbox")     // 共享 Sandbox 资源
  └─ PushLayer(new EditorLayer())
```

### EditorContext（编辑器共享状态）

```cpp
struct EditorContext {
    Ref<Scene> scene;              // 当前场景
    Entity selectedEntity;        // 当前选中实体
    Entity hoveredEntity;         // 鼠标悬停实体
    EditorCamera* editorCamera;   // 编辑器相机
    SignalBus signals;            // 面板间通信（类型安全的发布/订阅）
    CommandSystem* commands;      // Undo/Redo 命令系统
    EditorState state;            // Edit / Play / Simulate
    bool isEditorCameraActive;    // 视口是否聚焦
};
```

### SignalBus（类型安全的信号总线）

```cpp
class SignalBus {
    // 内部：unordered_map<type_index, vector<function<void(const void*)>>>
    template<typename T> void Publish(const T& event);
    template<typename T> void Subscribe(std::function<void(const T&)> handler);
};
```

编辑器事件：`EntitySelected`, `EntityHovered`, `EntityDestroyed`, `ComponentModified`, `SceneModified`, `ViewportResized`, `RequestNewScene`, `RequestOpenScene`, `RequestSaveScene`

### CommandSystem（Undo/Redo）

```cpp
class EditorCommand {
    virtual void Execute() = 0;
    virtual void Undo() = 0;
    virtual const char* GetName() = 0;
};

class CommandSystem {
    vector<Ref<EditorCommand>> m_History;
    int32_t m_CurrentIndex;
    static const int MAX_HISTORY = 50;
    void Execute(Ref<EditorCommand> cmd);
    void Undo(); void Redo();
};
```

已实现命令：`DestroyEntityCommand`、`SetTransformCommand`

### IPanel 接口

```cpp
class IPanel {
    virtual void OnAttach(EditorContext* context);
    virtual void OnDetach();
    virtual void OnUpdate(Timestep ts);
    virtual void OnUIRender() = 0;        // 必须在子类实现
    virtual void OnEvent(Event& event);
    bool IsOpen;
};
```

### 面板详情

| 面板 | 关键功能 |
|------|---------|
| **ViewportPanel** | Framebuffer 渲染 (RGBA8 + RED_INTEGER + DEPTH)、ImGuizmo (T/R/Y)、鼠标拾取 (ReadPixel RED_INTEGER)、Gizmo 变更生成 SetTransformCommand |
| **SceneHierarchyPanel** | 实体树、选择、添加/删除实体（右键菜单）、使用 DestroyEntityCommand |
| **PropertyPanel** | 编辑 Tag/Transform/SpriteRenderer/Camera/NativeScript、添加/删除组件、Transform 编辑创建 SetTransformCommand |
| **PerformancePanel** | FPS、帧时间、Renderer2D 统计 (DrawCalls, Quads, Vertices, TextureCount) |
| **ResourcePanel** | 扫描场景纹理使用、显示路径/尺寸/使用次数/缩略图 |

### 编辑器主循环

```
EditorLayer::OnUpdate(timestep)
  ├─ EditorCamera::OnUpdate()             // 鼠标/键盘控制
  ├─ 各 Panel OnUpdate()
  ├─ 同步 Camera 宽高比 (Viewport 尺寸)
  ├─ Viewport Framebuffer Bind
  ├─ Clear + ClearAttachment(1, -1)      // 清除颜色/实体 ID
  ├─ Scene::OnUpdate(timestep)           // 实际渲染
  └─ Framebuffer Unbind

EditorLayer::OnImguiRender()
  ├─ Dockspace + 菜单栏 (File/Edit)
  ├─ 工具栏 (Play/Stop/Simulate/Pause/Step)
  └─ 各 Panel OnUIRender() (Viewport → Hierarchy → Property → Resource → Performance)
```

---

## 8. 平台抽象

### 目录结构

```
Vulkitten/
├── Core/                        # 核心接口 (Window, IWindow, ISurface, Layer 等)
├── Window/Platform/Windows/     # Windows 平台窗口实现
│   ├── WindowsWindow.h/cpp      # GLFW 窗口，实现 Window + IWindow
│   ├── WindowsSurface.h/cpp     # ISurface 实现
│   ├── WindowsInput.h/cpp       # GLFW 输入
│   └── WindowsFileDialogs.cpp   # Win32 文件对话框
├── Renderer/                    # 渲染器抽象接口
│   ├── Backend/
│   │   ├── OpenGL/              # OpenGL 后端实现
│   │   │   ├── OpenGLContext.h/cpp      # GLAD 初始化 + SwapBuffers
│   │   │   ├── OpenGLRendererAPI.h/cpp  # RendererAPI 实现
│   │   │   ├── OpenGLBuffer.h/cpp       # Vertex/Index Buffer
│   │   │   ├── OpenGLVertexArray.h/cpp  # VAO
│   │   │   ├── OpenGLShader.h/cpp       # Shader 编译 + Uniform
│   │   │   ├── OpenGLTexture.h/cpp      # Texture2D
│   │   │   ├── OpenGLFramebuffer.h/cpp  # FBO
│   │   │   ├── OpenGLUtil.h/cpp         # GLenum 转换
│   │   │   └── OpenGLDevice.h/cpp       # IDevice 实现
│   │   └── Vulkan/              # Vulkan 后端 (待实现)
│   ├── RenderGraph/             # RenderGraph 系统
│   ├── Passes/                  # 渲染 Pass
│   └── ...
└── Scene/                       # ECS
```

### 关键抽象接口

```cpp
// Window.h — 抽象窗口接口（应用层使用）
class Window {
    static Scope<Window> Create(const WindowProps& props);
    virtual void OnUpdate() = 0;  // PollEvents + SwapBuffers
};

// IWindow.h — 平台窗口接口（后端使用）
// 与 Window 分离：Window 服务于应用层（事件、VSync），
// IWindow 服务于渲染后端（Surface 查询、交换链创建）
class IWindow {
    virtual SurfaceDesc GetSurfaceDesc() const = 0;  // 查询表面属性
    virtual ISurface* GetSurface() = 0;              // 获取平台绘制表面
};

// ISurface.h — 平台绘制表面抽象
// 封装 OS 原生窗口绘制区域。GL 下为 GLFWwindow 包装，
// Vulkan 下用于创建 VkSurfaceKHR
struct SurfaceDesc { uint32_t Width, Height; };
class ISurface {
    virtual SurfaceDesc GetDesc() const = 0;
    virtual void* GetNativeHandle() const = 0;  // HWND / GLFWwindow*
};

// Input.h — 抽象输入接口（静态单例）
class Input {
    static bool IsKeyPressed(KeyCode);  // 转发到平台实现
};

// GraphicsContext.h — 图形上下文
class GraphicsContext {
    virtual void Init() = 0;      // 初始化 API
    virtual void SwapBuffers() = 0;
};
```

---

## 9. 序列化与文件系统

### SceneSerializer（YAML）

```cpp
class SceneSerializer {
    Ref<Scene> m_Scene;
    void Serialize(const std::string& filepath);      // Scene → YAML 文件
    void SerializeEntity(YAML::Emitter&, Entity);     // 单个实体
    bool Deserialize(const std::string& filepath);    // YAML 文件 → Scene
};
```

### TexturePath 往返

`SpriteRendererComponent` 同时存储：
- `Ref<Texture2D> Texture` — 运行时纹理引用
- `std::string TexturePath` — 序列化路径（如 `"sandbox://assets/textures/Checkerboard.png"`）

反序列化：`sprite.Texture = Texture2D::Create(sprite.TexturePath);`

### 虚拟文件系统（实例化，通过 Engine 访问）

FileSystem 已从全静态类重构为实例类。Engine 持有 `Scope<FileSystem>` 实例，所有文件系统操作通过 `Engine::Get().GetFileSystem()` 访问。

```cpp
// 注册虚拟路径
Engine::Get().GetFileSystem().RegisterPath("../../Sandbox", "sandbox");

// 使用：
// "sandbox://assets/shaders/FlatColor.shader"
//   → Engine::Get().GetFileSystem().Resolve(path)
//   → "../../Sandbox/assets/shaders/FlatColor.shader"
```

协议前缀通过 `://` 分隔，解析为注册的物理路径。

---

## 10. 构建与依赖

### Vendored 依赖（Vulkitten/vendor/）

| 库 | 类型 | 版本获取 | 用途 |
|----|------|---------|------|
| **spdlog** | 编译 | git submodule | 高性能日志 |
| **glm** | 仅头文件 | git submodule | 数学库 (vec/mat/quat) |
| **glfw** | 编译 | git submodule | 窗口 + 输入 |
| **imgui** | 源文件包含 | git submodule | UI (Dear ImGui) |
| **entt** | 仅头文件 | git submodule | ECS 库 |
| **yaml-cpp** | 编译 | git submodule | YAML 序列化 |
| **ImGuizmo** | 源文件包含 | git submodule | 3D Gizmo (Translate/Rotate/Scale) |
| **imnodes** | 仅头文件 | git submodule | 节点编辑器 (未来使用) |
| **tinygltf** | 仅头文件 | git submodule | glTF 2.0 模型加载 |
| **nlohmann/json** | 仅头文件 | 手动 | JSON 解析 |
| **stb_image** | 仅头文件 | 手动 | 图像加载 |
| **glad** | 编译 | 手动 (Vulkitten/vendor/Glad/) | OpenGL 函数加载器 |

### 构建命令

```bash
# 快速构建
build.bat

# 手动构建
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Debug
```

---

## 延伸阅读

- [GOAL.md](../GOAL.md) — 项目目标、当前状态、路线图
- [AGENTS.md](../AGENTS.md) — 代码规范、构建命令、AI 代理开发指南
