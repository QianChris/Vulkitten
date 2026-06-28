# VulkittenRHI 设计文档

## 1. 架构概览

```
┌─────────────────────────────────────────────────┐
│               上层应用 (Sandbox, Editor)          │
├─────────────────────────────────────────────────┤
│              RenderCommandList                   │
│         (fluent API / 高层便捷接口)               │
├─────────────────────────────────────────────────┤
│  Renderer ─── IRenderDevice ─── ICommandBuffer   │
│  (帧生命周期)   (GPU 设备抽象)    (命令录制接口)     │
├───────────────┬─────────────────────────────────┤
│   GLDevice    │          VKDevice                │
│   GLCommandBuf│          VKCommandBuffer          │
│   GLResources │          VKResources              │
│   (OpenGL 4.6)│          (Vulkan 1.3)             │
└───────────────┴─────────────────────────────────┘
│              ResourceManager                     │
│       (Handle 池 + ABA 保护 + 资源生命周期)        │
└─────────────────────────────────────────────────┘
```

**核心原则：** 上层代码只看到抽象接口，不感知后端。切换 OpenGL ↔ Vulkan 改一行代码。

## 2. 资源模型

### Handle 系统

所有 GPU 资源通过类型安全的 Handle 引用，不暴露裸指针：

```cpp
BufferHandle   vb = device.CreateBuffer(desc, data);
TextureHandle  tex = device.CreateTexture(desc);
PipelineHandle pso = device.CreatePipeline(desc);
```

- `Handle<Tag>` 包含 `(id, generation)` 对
- `id=0` 表示 null handle
- generation 递增防止 ABA (use-after-free)
- 不同 Tag 类型之间零隐式转换

### ResourceManager

资源生命周期管理：
- **Handle 池**: 预分配 slot，复用已释放的 id
- **类型存储**: `unordered_map<uint32_t, unique_ptr<I*>>` 持有 RAII 资源
- **后端创建**: Device 调用 `AllocateSlot()` → 创建原生对象 → `Store*()` 存入
- **销毁**: `DestroyAll()` 在 shutdown 时递归释放所有资源

```
CreateBuffer(desc, data)
  → AllocateSlot()          // 从池中取 id
  → new GLBufferResource()  // 创建 RAII 原生对象
  → StoreBuffer(id, ptr)    // 存入 ResourceManager
  → return Handle{id, gen}  // 返回不透明句柄
```

## 3. 命令系统

### ICommandBuffer — 录制接口

所有 GPU 命令通过虚函数接口录制。录制顺序：

```
begin() → [barriers] → beginRenderPass() → bind*
       → draw / dispatch / copy → endRenderPass() → end()
```

关键命令：

| 类别 | 命令 |
|---|---|
| **Draw** | `Draw(vtxCount, firstVtx, instanceCount)` |
| | `DrawIndexed(idxCount, firstIdx, vtxOffset, instanceCount)` |
| | `DrawIndirect(buf, offset, drawCount, stride)` |
| **Compute** | `DispatchCompute(x, y, z)` |
| | `DispatchIndirect(buf, offset)` |
| **Debug** | `BeginDebugLabel(name, color)` |
| | `EndDebugLabel()` |
| | `InsertDebugMarker(name)` |

### OpenGL 后端 — 立即执行

- `glDrawArrays / glDrawElements / glDispatchCompute` 直接调用
- Barrier 基本是 no-op
- Debug 用 `GL_KHR_debug` (`glPushDebugGroup` / `glDebugMessageInsert`)

### Vulkan 后端 — 录制到 VkCommandBuffer

- 命令录制到 `VkCommandBuffer`，需要显式 `vkQueueSubmit` 执行
- Barrier 转为 `VkMemoryBarrier` / `VkImageMemoryBarrier`
- Debug 用 `VK_EXT_debug_utils` (`vkCmdBeginDebugUtilsLabelEXT`)

### RenderCommandList — 便捷层

对 `ICommandBuffer` 的 thin wrapper，提供：
- **Fluent API**: `cmd.BindPipeline(pso).BindGeometry(geo).Draw(3);`
- **高层封装**: `DrawMesh(geo, pso, bindings)` 自动处理全套绑定+绘制
- **逃生舱口**: `Cmd()` / `Device()` / `Resources()` 直接访问底层接口

## 4. 帧生命周期

```
Renderer::Create(config)
    ↓
┌─ BeginFrame() ─────────────────────┐
│  GL:  glfwMakeContextCurrent       │
│  VK:  swapchain.AcquireNextImage   │
│  → 创建 ICommandBuffer              │
│  → 创建 RenderCommandList          │
├────────────────────────────────────┤
│  录制命令...                        │
│  (draw / dispatch / copy / debug)  │
├─ EndFrame() ───────────────────────┤
│  GL:  glfwSwapBuffers              │
│  VK:  swapchain.SubmitAndPresent   │
│  → 销毁 command buffer              │
└────────────────────────────────────┘
```

## 5. Pipeline & Shader

### Shader 格式

两个后端共用 **SPIR-V** 二进制：
- Vulkan: 原生 `VkShaderModule`
- OpenGL 4.6: `GL_ARB_gl_spirv` + `glShaderBinary` + `glSpecializeShader`

测试也用 GLSL 源文件（GL 后端直接 `glCompileShader` 编译）。

### Pipeline 描述

```cpp
PipelineDesc desc;
desc.VertexShader   = vs;       // SPIR-V or GLSL
desc.FragmentShader = fs;
desc.ComputeShader  = cs;       // 如果有效 → compute pipeline
desc.VertexLayout   = { ... };  // 顶点属性布局
desc.RenderPass     = rp;
desc.Raster.Cull    = RasterState::CullMode::Back;
// ... depth, blend, buffer slots, texture slots
```

### 关键差异处理

| | OpenGL | Vulkan |
|---|---|---|
| **NDC Y 轴** | 向上 (Y-up) | 向下 (Y-down) |
| **Viewport** | `glViewport` | dynamic state |
| **Front Face** | CCW = front | CW = front (GL→VK: 翻转) |
| **Program** | `glCreateProgram` + link | `VkPipeline` (一次性) |
| **State** | 逐个设置 (全局状态机) | 全部预编译在 VkPipeline 里 |

## 6. 描述符绑定

用于 compute shader 和图形管线绑定 UBO/SSBO/纹理：

```cpp
// 图形管线声明
pipeDesc.BufferSlots = {
    {0, BufferSlot::Type::Uniform, ShaderStage::Vertex, sizeof(UBO)},
};
pipeDesc.TextureSlots = {
    {0, TextureSlot::Type::Sampled, ShaderStage::Fragment},
};

// 运行时绑定
cmd.BindUniformBuffer(0, ub, 0, sizeof(UBO));
cmd.BindTexture(0, tex, sampler);
```

**OpenGL**: 直接 `glBindBufferRange(GL_UNIFORM_BUFFER, slot, ...)`  
**Vulkan**: 通过 descriptor set (`VkDescriptorSet`) → `vkUpdateDescriptorSets` + `vkCmdBindDescriptorSets`

对于 compute pipeline 未显式声明 slot 的情况，VK 后端自动创建 slot 0-3 的 storage buffer 绑定。

### 6.1 Vulkan DescriptorSet 内部分配机制

#### 整体数据流

```
PipelineDesc (BufferSlots + TextureSlots)
       │
       ▼
VKPipelineResource 构造 (VKResources.cpp:340-373)
  → 从 slot 描述生成 VkDescriptorSetLayout bindings
  → vkCreateDescriptorSetLayout  →  每个 Pipeline 持有一个 layout
  → vkCreatePipelineLayout(setLayoutCount=1, 只用 set=0)
       │
       ▼
VKDevice::Init() (VKDevice.cpp:160)
  → CreateDescriptorPool()  全局唯一 VkDescriptorPool
       │
       ▼
运行时: VKCommandBuffer::WriteDescriptorSet() (VKCommandBuffer.cpp:269-313)
  → 查缓存  →  命中: 复用   /  未命中: vkAllocateDescriptorSets
  → vkUpdateDescriptorSets (写入 buffer info)
  → vkCmdBindDescriptorSets (绑定到 command buffer)
```

#### DescriptorPool — 全局单例

`VKDevice::CreateDescriptorPool()` ([VKDevice.cpp:483-502](src/vk/VKDevice.cpp#L483-L502)) 在设备初始化时创建**一个**全局池，生命周期与 device 相同，**从不 reset**：

| 描述符类型 | 池容量 |
|---|---|
| `VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER` | 16 |
| `VK_DESCRIPTOR_TYPE_STORAGE_BUFFER` | 8 |
| `VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER` | 16 |
| **maxSets** | **32** |

#### DescriptorSet 缓存与分配

缓存位于 `VKDevice::m_DescriptorSetCache`（[VKDevice.hpp:123](src/vk/VKDevice.hpp#L123)），`VKCommandBuffer` 通过指针共享。

**缓存键设计：**
```
cacheKey = (pipelineId << 8) | frameIndex
```
- 高 56 bits：Pipeline ID — 每个 pipeline 有独立的 descriptor set layout
- 低 8 bits：FrameIndex (0 或 1) — double-buffering in-flight frame 隔离

**分配流程（`WriteDescriptorSet`，[VKCommandBuffer.cpp:269-313](src/vk/VKCommandBuffer.cpp#L269-L313)）：**

1. 计算 `cacheKey`
2. 查 `m_DescriptorSetCache`：
   - **命中** → 复用已有 `VkDescriptorSet`（同一 pipeline + 同一 frame 内多次绑定共享）
   - **未命中** → 调用 `VKDevice::AllocateDescriptorSet(layout)` 从全局池分配新的 set，存入缓存
3. `vkUpdateDescriptorSets()` — 写入 buffer descriptor（即使缓存命中也每次都写）
4. `vkCmdBindDescriptorSets(set=0)` — 绑定到 command buffer

**生命周期：**
- DescriptorSet 随 `VkDescriptorPool` 销毁而自动回收（`vkDestroyDescriptorPool`）
- 缓存 `clear()` 在 `Shutdown()` 中先于 pool 销毁执行

#### 已知局限性

| 问题 | 说明 |
|---|---|
| **每次绑定都 Update** | 缓存命中时仍调用 `vkUpdateDescriptorSets`，对不变绑定是冗余开销 |
| **Pool 容量硬编码** | maxSets=32，pipeline 数 × 2 frame > 32 即耗尽 |
| **无池增长/Reset** | 耗尽后 `AllocateDescriptorSet` 返回 null，上层静默 `return` |
| **纹理绑定未实现** | `BindTexture` / `BindStorageTexture` 是空 stub（标记 `[HACK]`） |
| **单 Set 布局** | 只用 set=0，无法利用多 set 做频率分离（per-frame / per-material / per-draw） |
| **无 SPIR-V 反射** | Slot 声明依赖手动填写 `PipelineDesc::BufferSlots` / `TextureSlots` |

## 7. 资源创建速查

```cpp
// Buffer
BufferDesc bd;
bd.Size = 1024;
bd.Usage = BufferUsage::Vertex | BufferUsage::Storage;
bd.Memory = MemoryProperty::HostVisible;
auto buf = device.CreateBuffer(bd, initialDataPtr);

// Texture
TextureDesc td;
td.Type = TextureType::Texture2D;
td.Format = Format::RGBA8_UNORM;
td.Extent = {512, 512, 1};
td.Usage = TextureUsage::Sampled | TextureUsage::ColorAttachment;
auto tex = device.CreateTexture(td, pixelData);

// Geometry (纯 vertex/index buffer 引用，不含格式)
GeometryDesc gd;
gd.VertexBuffers[0] = vb;
gd.VertexBufferCount = 1;
gd.IndexBuffer = ib;
gd.IndexType = IndexType::UInt16;
gd.VertexCount = 100;
gd.IndexCount = 300;
auto geo = device.CreateGeometry(gd);
```

## 8. 目录结构

```
VulkittenRHI/
├── include/rhi/           # 公开头文件
│   ├── Core/              # Handle, Format, Types
│   ├── ICommandBuffer.hpp # 命令录制接口
│   ├── IRenderDevice.hpp  # 设备接口
│   ├── RenderCommandList.hpp # 便捷层
│   ├── Renderer.hpp       # 顶层入口
│   ├── ResourceDescs.hpp  # 所有描述符结构体
│   └── ResourceManager.hpp# Handle 池 + 资源存储
├── src/
│   ├── Renderer.cpp       # Renderer 工厂 + PIMPL
│   ├── RenderCommandList.cpp
│   ├── ResourceManager.cpp
│   ├── gl/                # OpenGL 4.6 后端
│   │   ├── GLDevice.*
│   │   ├── GLCommandBuffer.*
│   │   └── GLResources.*
│   └── vk/                # Vulkan 1.3 后端
│       ├── VKDevice.*
│       ├── VKCommandBuffer.*
│       ├── VKSwapchain.*
│       └── VKResources.*
├── samples/               # 示例 (triangle)
│   ├── main.cpp
│   └── CMakeLists.txt
├── tests/                 # 测试套件
│   ├── unit/              # Mock 单元测试 (无 GPU)
│   ├── gl/                # GL 集成测试 (需要 GPU)
│   ├── vk/                # VK 集成测试 (需要 GPU)
│   └── mocks/             # Mock 工具
├── CMakeLists.txt
├── README.md              # 快速开始
└── DESIGN.md              # 本文档
```

## 9. 已知限制 & 未来工作

- **Push Constants**: GL 后端尚未完整实现 (标记 `[HACK]`)
- **Copy Commands**: 两个后端都是 stub
- **Texture 附件绑定**: GL Framebuffer 的 attachment 绑定尚未完整
- **Descriptor Layout**: VK 后端用固定 slot 初始化，未做 SPIR-V 反射
- **Secondary Command Buffers**: 尚未实现
- **Async Compute / Ray Tracing / Mesh Shaders**: 标记 `[RESERVE]`
- **Memory**: 尚未实现 buffer/texture 间的 aliasing
