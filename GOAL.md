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

- **RenderGraph::Execute() 为空**：命令式渲染管线的数据结构齐全（RenderGraph、RenderPass、RenderCommand、ResourcePool、ResourceHandle），但 Execute 方法未实现，Pass 不会被真正执行
- **双渲染路径并存**：Scene::RenderScene() 通过 Renderer2D 实际完成渲染（可用）；RenderSystem 向 RenderGraph 添加命令但无效果。两条路径应统一
- **GPU 粒子绕过所有抽象**：直接在 GpuParticle.cpp 中使用 `glGenBuffers`、`glDispatchCompute`、SSBO 操作，未经过 RendererAPI 或 RenderGraph
- **Renderer::Render() 未被调用**：虽然调用了 `m_graph->Execute()`，但 `Renderer::Render()` 从未在主循环中被调用
- **QuadRenderPass 为空壳**：Passes/QuadRenderPass 无实际执行逻辑
- **Vulkan 后端未开始**：仅 API 枚举中存在（None, OpenGL），无 Vulkan 相关代码
- **Compute Pipeline 未抽象**：目前仅有 GPU 粒子直接使用 GL 计算管线，无通用抽象
- **imnodes 已 vendored 但未使用**：为未来的节点编辑器预留
- **无自动化测试**
- **仅支持 Windows x64 + OpenGL**

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
