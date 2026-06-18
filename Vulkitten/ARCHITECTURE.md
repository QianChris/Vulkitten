# ARCHITECTURE.md — Vulkitten 引擎设计与结构

## 1. 项目解剖

```
VulkittenEngine/
├── Vulkitten/              # 引擎核心 DLL
│   ├── src/
│   │   ├── Vulkitten.h     # 统一对外头文件
│   │   ├── vktpch.h/cpp    # 预编译头
│   │   └── Vulkitten/
│   │       ├── Core/       # 应用框架、窗口、输入、日志
│   │       ├── Events/     # 事件系统
│   │       ├── Renderer/   # 渲染器抽象 + RenderGraph + Renderer2D
│   │       ├── Scene/      # ECS（Scene, Entity, Components, Systems）
│   │       ├── ImGui/      # ImGui 集成层
│   │       ├── Perf/       # 性能分析（Instrumentor, Timer）
│   │       └── Utils/      # 工具（FileDialogs, YAMLConversions）
│   ├── Platform/
│   │   ├── Windows/        # GLFW 窗口 + 输入实现
│   │   └── OpenGL/         # OpenGL 渲染后端全实现
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

这是引擎最核心、最复杂的部分。当前存在**两条渲染路径**：

```
                    ECS (Components)
                         │
                         ├─────────────────────────────────┐
                         │                                 │
                    RenderSystem                     Scene::RenderScene()
                    (Path B: 新)                     (Path A: 旧/当前可用)
                         │                                 │
                         ▼                                 ▼
                    RenderGraph                        Renderer2D
                    (命令收集)                        (即时模式批处理)
                         │                                 │
                         ▼                                 ▼
                    Passes                            直接 OpenGL
                    (Execute() 为空!!)                 (实际工作路径)
                         │
                    [未来：任意后端]
                         │
                         ▼
                    实际 Draw Call
```

### 六层抽象（自上而下）

| 层 | 类/文件 | 职责 | 状态 |
|----|---------|------|------|
| **6. RenderSystem** | `Scene/Systems/RenderSystem` | ECS→RenderCommand 桥接，遍历实体生成 DrawQuadCommand | ✅ 可用，输出进入 Path B |
| **5. RenderGraph** | `Renderer/RenderGraph/RenderGraph` | 命令式延迟渲染管线，管理 Passes 和 FrameCommands | 🚧 Execute() 为空 |
| **4. Renderer** | `Renderer/Renderer` | 场景级抽象 (BeginScene/EndScene/Submit)，持有 RenderGraph 实例 | ✅ 可用但不被调用 |
| **3. Renderer2D** | `Renderer/Renderer2D` | 即时模式批处理四边形渲染器 (10000 四边形/32 纹理槽) | ✅ 可用，Path A |
| **2. Legacy::RenderCommand** | `Renderer/RenderCommand` | 静态代理类包装 RendererAPI* 单例，提供内联 Clear/Draw 方法 | ✅ 广泛使用 |
| **1. RendererAPI** | `Renderer/RendererAPI` | 最低层 GPU 抽象：Init, Clear, DrawIndexed 等纯虚接口 | ✅ OpenGL 实现 |

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

> **状态**：数据结构完整，Execute() 未实现。这是 Phase 1 的核心工作。

### 核心数据结构

```
RenderGraph
├── vector<RenderPass> m_Passes         // 已注册的 Pass
├── vector<RenderCommand> m_FrameCommands  // 帧内所有渲染命令
├── AddPass(pass)                       // 注册 Pass
├── AddCommand(cmd)                     // 收集命令（由 RenderSystem 调用）
├── Execute()                           // 🚧 空实现，需完成
└── Clear()                             // 每帧清空命令
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

1. `RenderGraph::Execute()` 需要对每个 Pass 过滤匹配的 Commands，调用其 `onExecute`
2. `ResourcePool` 需要在 Execute 期间分配/转换实际 GPU 资源
3. Pass 的资源依赖分析和 barrier 插入（目前仅在 PassResourceUsage 中声明）
4. QuadRenderPass::onExecute 需要实际调用 Renderer2D 来绘制四边形

---

## 6. GPU 粒子系统

> **注意**：该系统绕过所有渲染器抽象，直接使用 OpenGL 调用。这是架构债，需在后续 Phase 中解决。

### 架构

```
GpuEmitterManager (Scene 持有, GpuParticle/GpuParticle.h)
├── ShaderLibrary (持有 compute shader 和 render shader)
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
Platform/
├── Windows/
│   ├── WindowsWindow.h/cpp     # GLFW 窗口实现，实现 Window 接口
│   ├── WindowsInput.h/cpp      # GLFW 输入实现，实现 Input 接口（单例）
│   └── WindowsFileDialogs.cpp  # Win32 文件对话框
└── OpenGL/
    ├── OpenGLContext.h/cpp      # GLAD 初始化 + SwapBuffers
    ├── OpenGLRendererAPI.h/cpp  # RendererAPI 实现 (glClear, glDrawElements…)
    ├── OpenGLBuffer.h/cpp       # VertexBuffer + IndexBuffer (GLuint)
    ├── OpenGLVertexArray.h/cpp  # VAO (glGenVertexArrays, vertex attrib)
    ├── OpenGLShader.h/cpp       # Shader 编译 + Uniform 上传 (含 #include 预处理)
    ├── OpenGLTexture.h/cpp      # Texture2D (stb_image 加载)
    ├── OpenGLFramebuffer.h/cpp  # FBO (含 MSAA 支持)
    └── OpenGLUtil.h/cpp         # ShaderDataType → GLenum 转换
```

### 关键抽象接口

```cpp
// Window.h — 抽象窗口接口
class Window {
    static Scope<Window> Create(const WindowProps& props);
    virtual void OnUpdate() = 0;  // PollEvents + SwapBuffers
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

### 虚拟文件系统

```cpp
FileSystem::RegisterPath("../../Sandbox", "sandbox");
FileSystem::RegisterPath("../../VulkittenEditor", "editor");
FileSystem::RegisterPath("../../VulkittenEditor/assets/icons", "editorIcons");

// 使用时：
// "sandbox://assets/shaders/FlatColor.shader" → "../../Sandbox/assets/shaders/FlatColor.shader"
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
