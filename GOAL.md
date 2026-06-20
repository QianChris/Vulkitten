# GOAL.md — Vulkitten Engine 项目目标与状态

## 项目愿景

Vulkitten 是一个个人搭建的 C++17 玩具渲染引擎，核心目的是**探索现代引擎架构**。项目的长期目标是成为一个 Editor 工具，支持对 Compute Shader 的高效开发与性能可视化。

引擎采用 **ECS 前端 + 抽象 Renderer 后端** 的分离式架构：
- **前端**：基于 EnTT 的 ECS，通过 System 将 RenderCommand 提交到后端
- **后端**：基于 RenderGraph 的 Pass 管线，在 Pass 中完成真正的后端 API 调用
- **目标**：架构上足够分离，使 Renderer 本身最终也能成为插件，后续方便加入 Vulkan 后端、物理、动画等插件

虽然名为 "Vulkitten"，目前仅实现了 OpenGL 后端，但设计上预留了 Vulkan 适配空间。

---

## 当前状态

### 项目结构
| 目标 | 类型 | 说明 |
|------|------|------|
| `Vulkitten` | 动态库 (DLL) | 引擎核心：ECS、渲染器抽象、OpenGL 后端、ImGui、事件系统 |
| `Sandbox` | 可执行文件 | 简单应用层，用于测试引擎功能 |
| `VulkittenEditor` | 可执行文件 | 编辑器：场景层级、属性面板、视口(Gizmo+拾取)、性能面板、资源面板 |

### ✅ 已完成/可用的功能

**核心框架：**
- Application/Layer/LayerStack 应用框架，支持 push/pop layer 和 overlay
- 自定义事件系统（Event + EventDispatcher），支持键盘、鼠标、窗口事件
- GLFW 窗口管理 + Windows 输入抽象
- spdlog 日志系统，支持引擎级 `VKT_CORE_*` 和客户端 `VKT_*` 宏
- 虚拟文件系统（FileSystem::RegisterPath），协议前缀映射到物理路径
- 性能分析宏（VKT_PROFILE_FUNCTION / VKT_PROFILE_RENDER_FUNCTION）

**ECS：**
- EnTT 作为 ECS 后端，Scene 持有 `entt::registry`
- Entity 封装 `entt::entity`，提供 AddComponent/GetComponent/HasComponent/RemoveComponent
- 组件：TagComponent、TransformComponent（含脏标记 + 缓存矩阵）、SpriteRendererComponent、CameraComponent、NativeScriptComponent、GpuEmitterComponent
- System 抽象接口，Scene 持有 `vector<Scope<System>>`
- RenderSystem：遍历 ECS 实体，生成 DrawQuadCommand 提交到 RenderGraph

**渲染器：**
- 六层渲染器抽象（见 [ARCHITECTURE.md](Vulkitten/ARCHITECTURE.md) 第 4 节）
- OpenGL 后端：完整实现 RendererAPI、Buffer、VertexArray、Shader、Texture2D、Framebuffer
- Renderer2D：即时模式批处理 2D 四边形渲染器（最多 10000 四边形，32 纹理槽）
- 抽象资源工厂模式：`Texture2D::Create()` 根据 `RendererAPI::GetAPI()` 返回平台实现
- Shader 系统：从 JSON 描述文件加载，支持 GLSL `#include` 预处理
- Framebuffer：支持 MSAA、多附件（颜色 + RED_INTEGER 实体拾取 + 深度）
- **当前实际渲染路径**：Scene::RenderScene() → Renderer2D::BeginScene/DrawQuad/EndScene → OpenGL

**GPU 粒子系统：**
- 直接使用 OpenGL Compute Shader（双缓冲 SSBO + Indirect Dispatch/Draw）
- 每帧 4 个计算 Pass：SimArg → Sim → Emit → RenderArg → glDrawArraysIndirect(GL_POINTS)
- 注意：**绕过所有渲染器抽象**，使用原始 GL 调用

**编辑器：**
- 5 个停靠面板（基于 ImGui Docking）：Viewport、SceneHierarchy、Property、Performance、Resource
- Viewport 支持 Framebuffer 渲染 + ImGuizmo (Translate/Rotate/Scale) + 鼠标实体拾取
- EditorContext + SignalBus 解耦面板间通信
- CommandSystem 支持 Undo/Redo（最多 50 条历史记录）
- 编辑器场景状态机：Edit / Play / Simulate
- YAML 场景序列化/反序列化（SceneSerializer）

### 🚧 进行中/已知问题

- **GPU 粒子绕过所有抽象**：直接在 GpuParticle.cpp 中使用原始 GL 调用，未经过 RendererAPI 或 RenderGraph
- **双渲染路径已统一**：Scene::OnUpdate 始终走 RenderGraph 路径，Scene::RenderScene 已移除。ExampleLayer2 完全依赖 RenderGraph 进行渲染
- **Vulkan 后端未开始**：仅 API 枚举中存在（None, OpenGL），无 Vulkan 相关代码
- **Compute Pipeline 未抽象**：目前仅有 GPU 粒子直接使用 GL 计算管线，无通用抽象
- **imnodes 已 vendored 但未使用**：为未来的节点编辑器预留
- **无自动化测试**
- **仅支持 Windows x64 + OpenGL**

### ✅ Task 2 已完成 (2026-06-19)

- RenderGraph 新增 `SetFramebuffer` / `GetFramebuffer`，Pass 可在 nullptr（默认 Backbuffer）或自定义 Framebuffer 间切换
- PreparePass 和 SpriteRenderPass 在 Framebuffer 配置时自动绑定/解绑
- EditorLayer 通过 `Renderer::GetRenderGraph()->SetFramebuffer()` 配置视口 Framebuffer，不再手动 Bind/Clear/Unbind

### ✅ Task 3 已完成 (2026-06-19)

- 移除 Scene::RenderScene() 和旧回退路径，所有渲染统一走 RenderGraph
- 创建 GpuParticlePass，GPU 粒子 Update+Render 在 PreparePass 和 SpriteRenderPass 之间执行
- Scene 移除 Renderer2D.h 依赖

### ✅ Task 4 已完成 (2026-06-19)

- 创建 ClassFactory（Meyer's Singleton），作为引擎唯一集中式访问点
- 提供 UUID 生成（GenerateUUID）、GetApplication、GetWindow、GetRenderGraph
- 已加入 Vulkitten.h 统一头文件

### ✅ Task 5 已完成 (2026-06-19)

- RenderGraph 新增 GetPassCount() 和 GetPassName(uint32_t index) 公开方法
- 外部代码（调试 UI、编辑器面板等）可查询已注册 Pass 的数量和名称
- GetPassName 包含 VKT_CORE_ASSERT 边界检查
- **Compute Pipeline 未抽象**：目前仅有 GPU 粒子直接使用 GL 计算管线，无通用抽象
- **imnodes 已 vendored 但未使用**：为未来的节点编辑器预留
- **无自动化测试**
- **仅支持 Windows x64 + OpenGL**

### ✅ Phase: 引擎资产管理 (Task 1-7, 2026-06-20)

所有 7 个任务已完成，覆盖五大功能：

1. **引擎 Shader 资产化** — `Vulkitten/assets/` 目录结构已建立，所有引擎 Pass 用的 shader 和 compute shader 已从 Sandbox 迁移。注册了 `engine://` 虚拟路径。
2. **ShaderLibrary 引擎独有** — ShaderLibrary 从 GpuEmitterManager 移至 RenderContext 统一管理，构造器私有化（仅 friend RenderContext），用户仅能通过 `RenderContext::Get().GetShaderLibrary()` 获取引用并加载自有 shader。
3. **GpuResourceManager GC** — 添加 TickFrame/Gc 机制，基于帧计数器和 external tracker (weak_ptr) 自动回收未被外部持有的 GPU 资源。
4. **Buffer/Texture 统一管理** — Texture2D::Create 和 Buffer::Create 工厂方法在创建平台资源后自动向 GpuResourceManager 注册，提供统一的生命周期追踪。
5. **Framebuffer Key-Value 模式** — RenderGraph 改为 Key→Framebuffer 映射表，Pass 持有 Graph 指针按 Key 获取 FB。Execute 不再自动 Bind/Unbind。窗口 Resize 统一更新所有已注册 FB。EditorLayer 简化为 `graph->SetFramebuffer("Viewport", fb)` 一行配置。

### ✅ Task 1 已完成 (2026-06-19)

- RenderGraph::Execute() 已实现，按 Pass 名称过滤命令 → 调用 onExecute
- PreparePass 处理 ClearCommand，SpriteRenderPass 处理 DrawQuadCommand，EndPass 处理 SwapBuffers
- Renderer::Render() 已接入 Application 主循环（ImGui 之后，PollEvents 之前）
- Scene::OnUpdate 在有 System 时设置 Camera/VP 到 RenderGraph 并跳过直接 RenderScene
- SwapBuffers 从 WindowsWindow::OnUpdate 移至 EndPass
- QuadRenderPass 已替换为 SpriteRenderPass

---

## 路线图

### Phase 1 — 完成 RenderGraph 管线（当前重点）
- 实现 `RenderGraph::Execute()`：遍历 Passes，按 Pass 过滤 Commands，调度 ExecuteFunc 回调
- 将 `Renderer::Render()` 接入主循环
- 实现 QuadRenderPass::onExecute，内部通过 Renderer2D 执行 DrawQuadCommand
- 迁移 Scene::RenderScene() 使用 RenderGraph 路径
- 移除或条件化旧的直接 Renderer2D 路径
- 将 GpuParticle 的渲染接入 RenderGraph（作为 ParticlePass）

### Phase 2 — Shader 管线和材质系统
- 设计跨 API Shader 格式（以 Vulkan SPIR-V 为规范，OpenGL 编译兼容）
- 创建 Material 资源：Shader + Uniform 绑定
- 抽象 Compute Shader 加载（当前仅在 GPU 粒子中硬编码）

### Phase 3 — ECS 集成与更多 System
- 实现 `Scene::OnUpdateRuntime()`（当前为空桩）
- 添加 TransformSystem（层级变换更新）
- 添加 CameraSystem（相机管理、视锥剔除）
- System 执行顺序可配置

### Phase 4 — 插件架构
- 定义插件接口（IRendererPlugin、IPhysicsPlugin 等）
- 将 RendererAPI/OpenGLRendererAPI 抽象为可加载的插件后端
- 设计插件加载/卸载生命周期（预留热重载能力）
- 将整个 Renderer 变为引擎的一个插件

### Phase 5 — Vulkan 后端
- 实现 VkRendererAPI、VkBuffer、VkTexture、VkShader、VkVertexArray、VkFramebuffer
- 利用 RenderGraph 已有的 Vulkan 导向设计（AccessFlags、pipelineStages、imageLayout）
- RenderGraphResource 已包含 Vulkan 概念（TextureDesc、BufferDesc、Swapchain 感知）

### Phase 6 — 通用 Compute Pipeline
- 超越 GPU 粒子，抽象通用 Compute Shader 调度
- 将 Compute 作为 RenderGraph 的一种 Pass 类型
- 支持 Compute Pass 对 RenderGraph 资源的读写访问

### Phase 7 — 编辑器增强
- 基于 imnodes 的 Compute Shader 节点编辑器
- 性能可视化工具（GPU 计时、Draw Call 检查器）
- 编辑器内 Material/Shader 编辑
- 场景调试视图（物理碰撞体等）
- 更多渲染方法支持（延迟渲染、后处理等）
