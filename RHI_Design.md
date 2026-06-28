# RHI (Rendering Hardware Interface) 设计文档

> 设计原则：以 Vulkan 显式模型为基准，OpenGL 通过适配层模拟。  
> 目标：支撑 RenderGraph、Compute Shader Lab、多后端（GL/VK/DX/Metal）。

---

## 目录

1. [Core/Handle.hpp](#1-corehandlehpp) — 强类型资源句柄
2. [Core/Format.hpp](#2-coreformathpp) — 跨 API 像素/顶点格式
3. [Core/Types.hpp](#3-coretypeshpp) — 基础类型与枚举
4. [ISurface.hpp](#4-isurfacehpp) — 平台表面抽象
5. [FrameContext.hpp](#5-framecontexthpp) — 每帧执行上下文
6. [IRenderDevice.hpp](#6-irenderdevicehpp) — 设备总接口
7. [ICommandBuffer.hpp](#7-icommandbufferhpp) — 命令录制接口
8. [IBuffer.hpp](#8-ibufferhpp) — GPU 缓冲区
9. [ITexture.hpp](#9-itexturehpp) — GPU 纹理
10. [IShader.hpp](#10-ishaderhpp) — 着色器模块
11. [IPipeline.hpp](#11-ipipelinehpp) — 图形/计算管线
12. [IGeometry.hpp](#12-igeometryhpp) — 几何数据（VBO+IBO）
13. [IRenderPass.hpp](#13-irenderpasshpp) — 渲染通道描述
14. [ISampler.hpp](#14-isamplerhpp) — 纹理采样器
15. [Renderer.hpp](#15-rendererhpp) — 总入口与配置

---

## 1. Core/Handle.hpp

**重要性：★★★★★** — 所有 GPU 资源的基石。禁止裸指针传递。

```cpp
#pragma once
#include <cstdint>
#include <functional>

namespace rhi {

// 强类型句柄，编译期防混用
// 例：BufferHandle 不能隐式转成 TextureHandle
template <typename Tag>
class Handle {
public:
    Handle() = default;
    explicit Handle(uint32_t id) : m_id(id) {}

    uint32_t id() const { return m_id; }
    bool valid() const { return m_id != 0; }  // 0 为 Null Handle

    bool operator==(Handle other) const { return m_id == other.m_id; }
    bool operator!=(Handle other) const { return m_id != other.m_id; }
    bool operator<(Handle other) const { return m_id < other.m_id; }

    // 用于 unordered_map
    struct Hash {
        size_t operator()(Handle h) const { return std::hash<uint32_t>{}(h.m_id); }
    };

private:
    uint32_t m_id = 0;
};

// 资源标签（空结构体，仅用于类型区分）
struct BufferTag {};
struct TextureTag {};
struct ShaderTag {};
struct PipelineTag {};
struct GeometryTag {};
struct SamplerTag {};
struct RenderPassTag {};
struct FramebufferTag {};

// 强类型别名
using BufferHandle     = Handle<BufferTag>;
using TextureHandle    = Handle<TextureTag>;
using ShaderHandle     = Handle<ShaderTag>;
using PipelineHandle   = Handle<PipelineTag>;
using GeometryHandle   = Handle<GeometryTag>;
using SamplerHandle    = Handle<SamplerTag>;
using RenderPassHandle = Handle<RenderPassTag>;
using FramebufferHandle= Handle<FramebufferTag>;

} // namespace rhi
```

---

## 2. Core/Format.hpp

**重要性：★★★★★** — 跨 API 的像素/顶点格式统一表达。Vulkan 与 OpenGL 的格式转换在此定义。

```cpp
#pragma once
#include <cstdint>

namespace rhi {

// 像素/顶点/深度/压缩格式
// 命名规则：[Component][BitsPerChannel]_[Type] 或压缩格式名
enum class Format {
    Unknown,

    // 8-bit UNORM
    R8_UNORM,
    RG8_UNORM,
    RGBA8_UNORM,
    BGRA8_UNORM,
    RGBA8_SRGB,
    BGRA8_SRGB,

    // 16-bit Float / UNORM
    R16_FLOAT,
    RG16_FLOAT,
    RGBA16_FLOAT,
    R16_UNORM,

    // 32-bit Float
    R32_FLOAT,
    RG32_FLOAT,
    RGB32_FLOAT,
    RGBA32_FLOAT,

    // 32-bit Integer（常用于原子操作、ID缓冲）
    R32_UINT,
    R32_SINT,
    RG32_UINT,
    RGBA32_UINT,

    // 深度/模板
    D16_UNORM,
    D32_FLOAT,
    D24_UNORM_S8_UINT,
    D32_FLOAT_S8_UINT,

    // BC压缩（可选，后续扩展）
    BC1_RGB_UNORM,
    BC3_RGBA_UNORM,
    BC5_RG_UNORM,
    BC7_RGBA_UNORM,
};

// 每像素字节数
inline uint32_t formatByteSize(Format f) {
    switch (f) {
        case Format::R8_UNORM:        return 1;
        case Format::RG8_UNORM:       return 2;
        case Format::RGBA8_UNORM:
        case Format::BGRA8_UNORM:
        case Format::RGBA8_SRGB:
        case Format::BGRA8_SRGB:
        case Format::R32_FLOAT:
        case Format::R32_UINT:
        case Format::R32_SINT:        return 4;
        case Format::RG32_FLOAT:
        case Format::RG32_UINT:       return 8;
        case Format::RGB32_FLOAT:     return 12;
        case Format::RGBA32_FLOAT:
        case Format::RGBA32_UINT:     return 16;
        case Format::R16_FLOAT:       return 2;
        case Format::RG16_FLOAT:      return 4;
        case Format::RGBA16_FLOAT:    return 8;
        case Format::D16_UNORM:       return 2;
        case Format::D32_FLOAT:       return 4;
        case Format::D24_UNORM_S8_UINT:
        case Format::D32_FLOAT_S8_UINT: return 4;  // 实际可能 4~8，按 packed 算
        default: return 0;
    }
}

// 通道数
inline uint32_t formatComponentCount(Format f) {
    switch (f) {
        case Format::R8_UNORM:
        case Format::R16_FLOAT:
        case Format::R32_FLOAT:
        case Format::R32_UINT:
        case Format::R32_SINT:
        case Format::D16_UNORM:
        case Format::D32_FLOAT:       return 1;
        case Format::RG8_UNORM:
        case Format::RG16_FLOAT:
        case Format::RG32_FLOAT:
        case Format::RG32_UINT:
        case Format::BC5_RG_UNORM:    return 2;
        case Format::RGB32_FLOAT:     return 3;
        case Format::RGBA8_UNORM:
        case Format::BGRA8_UNORM:
        case Format::RGBA8_SRGB:
        case Format::BGRA8_SRGB:
        case Format::RGBA16_FLOAT:
        case Format::RGBA32_FLOAT:
        case Format::RGBA32_UINT:
        case Format::BC3_RGBA_UNORM:
        case Format::BC7_RGBA_UNORM:  return 4;
        default: return 0;
    }
}

// 是否为深度/模板格式
inline bool isDepthFormat(Format f) {
    return f == Format::D16_UNORM || f == Format::D32_FLOAT ||
           f == Format::D24_UNORM_S8_UINT || f == Format::D32_FLOAT_S8_UINT;
}

inline bool isStencilFormat(Format f) {
    return f == Format::D24_UNORM_S8_UINT || f == Format::D32_FLOAT_S8_UINT;
}

} // namespace rhi
```

---

## 3. Core/Types.hpp

**重要性：★★★★☆** — 渲染通用数据结构，零 API 依赖。

```cpp
#pragma once
#include <cstdint>
#include <array>

namespace rhi {

struct Extent2D {
    uint32_t width = 0;
    uint32_t height = 0;
};

struct Extent3D {
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t depth = 1;
};

struct Offset2D {
    int32_t x = 0;
    int32_t y = 0;
};

struct Offset3D {
    int32_t x = 0;
    int32_t y = 0;
    int32_t z = 0;
};

struct Rect2D {
    Offset2D offset;
    Extent2D extent;
};

struct Viewport {
    float x = 0.0f;
    float y = 0.0f;
    float width = 0.0f;
    float height = 0.0f;
    float minDepth = 0.0f;
    float maxDepth = 1.0f;
};

struct ClearColor {
    std::array<float, 4> rgba = {0.0f, 0.0f, 0.0f, 1.0f};
};

struct ClearDepthStencil {
    float depth = 1.0f;
    uint32_t stencil = 0;
};

union ClearValue {
    ClearColor color;
    ClearDepthStencil depthStencil;
};

// 着色器阶段
enum class ShaderStage : uint32_t {
    Vertex   = 1 << 0,
    Fragment = 1 << 1,
    Compute  = 1 << 2,
    // Geometry = 1 << 3,  // 暂不启用
};

inline ShaderStage operator|(ShaderStage a, ShaderStage b) {
    return static_cast<ShaderStage>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

// 索引类型
enum class IndexType {
    UInt16,
    UInt32,
};

// 缓冲区用途（可组合）
enum class BufferUsage : uint32_t {
    None        = 0,
    Vertex      = 1 << 0,
    Index       = 1 << 1,
    Uniform     = 1 << 2,
    Storage     = 1 << 3,  // SSBO / UAV
    TransferSrc = 1 << 4,
    TransferDst = 1 << 5,
};

inline BufferUsage operator|(BufferUsage a, BufferUsage b) {
    return static_cast<BufferUsage>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}
inline bool hasUsage(BufferUsage flags, BufferUsage flag) {
    return (static_cast<uint32_t>(flags) & static_cast<uint32_t>(flag)) != 0;
}

// 纹理用途（可组合）
enum class TextureUsage : uint32_t {
    None                = 0,
    Sampled             = 1 << 0,  // Shader 采样
    ColorAttachment     = 1 << 1,
    DepthStencilAttachment = 1 << 2,
    Storage             = 1 << 3,  // ImageStore / UAV
    TransferSrc         = 1 << 4,
    TransferDst         = 1 << 5,
};

inline TextureUsage operator|(TextureUsage a, TextureUsage b) {
    return static_cast<TextureUsage>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

// 内存属性（提示后端如何分配）
enum class MemoryProperty : uint32_t {
    DeviceLocal     = 1 << 0,  // GPU 显存，CPU 不可见
    HostVisible     = 1 << 1,  // CPU 可写，如 staging buffer
    HostCoherent    = 1 << 2,  // 无需 flush（GL 忽略）
    HostCached      = 1 << 3,  // CPU 回读优化（GL 忽略）
};

} // namespace rhi
```

---

## 4. ISurface.hpp

**重要性：★★★★☆** — 窗口与后端的唯一接触点。后端不认识 GLFW/SDL/Win32。

```cpp
#pragma once
#include "Core/Types.hpp"
#include "Core/Format.hpp"

namespace rhi {

// 平台层提供的 Surface 描述
// 后端只读此描述，不持有窗口句柄
struct SurfaceDesc {
    void* nativeWindowHandle = nullptr;  // HWND / NSWindow / X11 Window / wl_surface
    void* nativeDisplayHandle = nullptr; // X11 Display / wl_display（部分平台需要）
    uint32_t width = 0;
    uint32_t height = 0;
    Format preferredFormat = Format::BGRA8_UNORM;  // 或 RGBA8_UNORM
    bool vsync = true;
};

// 抽象 Surface
// 由 Platform 层实现，传给 IRenderDevice 初始化
class ISurface {
public:
    virtual ~ISurface() = default;

    virtual void getSize(uint32_t& width, uint32_t& height) const = 0;
    virtual Format getFormat() const = 0;

    // 平台相关的 present 调用
    // OpenGL: 内部调用 SwapBuffers
    // Vulkan: 由 VKDevice 的 endFrame 调用 vkQueuePresentKHR，ISurface 仅提供 VkSurfaceKHR
    virtual void present() = 0;
};

} // namespace rhi
```

---

## 5. FrameContext.hpp

**重要性：★★★★★** — 每帧的"执行上下文"。隐藏 Fence/Semaphore/CommandBuffer 等同步对象。

```cpp
#pragma once
#include <cstdint>

namespace rhi {

// 每帧由 IRenderDevice::beginFrame() 产出，endFrame() 消费
// 上层（RenderGraph / RenderContext）只读 frameIndex 和 swapchainIndex
// 所有同步细节（Fence / Semaphore / CommandPool）由后端内部管理
struct FrameContext {
    uint32_t frameIndex = 0;      // Ring Buffer 索引（0 ~ MAX_FRAMES_IN_FLIGHT-1）
    uint32_t swapchainIndex = 0;  // Swapchain Image 索引（如 0/1/2）

    // 类型擦除的内部指针，仅供后端实现使用
    // GL: 可能为 nullptr
    // VK: 指向 VKFrameContext 内部结构
    void* internal = nullptr;
};

} // namespace rhi
```

---

## 6. IRenderDevice.hpp

**重要性：★★★★★** — 后端总接口。资源创建 + 帧生命周期 + 命令缓冲分配。

```cpp
#pragma once
#include "Core/Handle.hpp"
#include "Core/Types.hpp"
#include "FrameContext.hpp"
#include "ISurface.hpp"
#include <memory>

namespace rhi {

// 前向声明
class ICommandBuffer;

// 设备配置
struct DeviceConfig {
    SurfaceDesc surface;
    bool enableValidation = true;
    bool enableDebugMarkers = true;  // 调试标签（如 VK_EXT_debug_utils）
    uint32_t framesInFlight = 2;     // Ring Buffer 大小（2 或 3）
};

// 渲染设备抽象
// 生命周期：应用级
class IRenderDevice {
public:
    virtual ~IRenderDevice() = default;

    // === 帧生命周期 ===
    // beginFrame: 等待本 slot 的上一帧完成，acquire swapchain image，开始录制
    // endFrame:   结束录制，submit，present
    virtual FrameContext beginFrame() = 0;
    virtual void endFrame(FrameContext ctx) = 0;

    // === 命令缓冲 ===
    // 从 FrameContext 关联的 CommandPool 分配
    virtual std::unique_ptr<ICommandBuffer> createCommandBuffer(FrameContext ctx) = 0;

    // === 资源创建 ===
    // 所有资源返回 Handle，由 ResourceManager 包装后提供给上层
    // 底层实现可直接调用，但建议统一走 ResourceManager

    virtual BufferHandle   createBuffer(const struct BufferDesc& desc, const void* initialData = nullptr) = 0;
    virtual TextureHandle  createTexture(const struct TextureDesc& desc, const void* initialData = nullptr) = 0;
    virtual ShaderHandle   createShader(ShaderStage stage, std::span<const uint8_t> bytecode) = 0;
    virtual PipelineHandle createPipeline(const struct PipelineDesc& desc) = 0;
    virtual GeometryHandle createGeometry(const struct GeometryDesc& desc) = 0;
    virtual SamplerHandle  createSampler(const struct SamplerDesc& desc) = 0;

    // === RenderPass / Framebuffer（可选，后端内部缓存） ===
    // 若后端需要显式 RenderPass（VK），则内部缓存；GL 可忽略
    virtual RenderPassHandle   createRenderPass(const struct RenderPassDesc& desc) = 0;
    virtual FramebufferHandle  createFramebuffer(const struct FramebufferDesc& desc) = 0;

    // === 窗口响应 ===
    virtual void onResize(uint32_t width, uint32_t height) = 0;

    // === 工具 ===
    virtual void waitIdle() = 0;  // 强制 GPU 完成所有工作（析构/重建 swapchain 时用）
    virtual void* getNativeDevice() = 0;  // 类型擦除：VK 返回 VkDevice，GL 返回 nullptr
};

} // namespace rhi
```

---

## 7. ICommandBuffer.hpp

**重要性：★★★★★** — 命令录制接口。Vulkan 是真录制，OpenGL 可缓存后批量刷或直接执行。

```cpp
#pragma once
#include "Core/Handle.hpp"
#include "Core/Types.hpp"
#include "Core/Format.hpp"
#include <span>

namespace rhi {

// 管线阶段（用于 Barrier 的 srcStage / dstStage）
enum class PipelineStage : uint32_t {
    TopOfPipe              = 1 << 0,
    DrawIndirect           = 1 << 1,
    VertexInput            = 1 << 2,
    VertexShader           = 1 << 3,
    FragmentShader         = 1 << 4,
    EarlyFragmentTests     = 1 << 5,
    LateFragmentTests      = 1 << 6,
    ColorAttachmentOutput  = 1 << 7,
    ComputeShader          = 1 << 8,
    Transfer               = 1 << 9,
    BottomOfPipe           = 1 << 10,
    Host                   = 1 << 11,
};

// 访问掩码（用于 Barrier 的 srcAccess / dstAccess）
enum class AccessFlags : uint32_t {
    None                     = 0,
    IndirectCommandRead      = 1 << 0,
    IndexRead                = 1 << 1,
    VertexAttributeRead      = 1 << 2,
    UniformRead              = 1 << 3,
    ShaderRead               = 1 << 4,
    ShaderWrite              = 1 << 5,
    ColorAttachmentRead      = 1 << 6,
    ColorAttachmentWrite     = 1 << 7,
    DepthStencilAttachmentRead  = 1 << 8,
    DepthStencilAttachmentWrite = 1 << 9,
    TransferRead             = 1 << 10,
    TransferWrite            = 1 << 11,
    MemoryRead               = 1 << 12,
    MemoryWrite              = 1 << 13,
};

// Image 布局（Vulkan 核心概念，OpenGL 内部忽略）
enum class ImageLayout {
    Undefined,
    General,
    ColorAttachment,
    DepthStencilAttachment,
    DepthStencilReadOnly,
    ShaderRead,
    TransferSrc,
    TransferDst,
    PresentSrc,
};

// 命令缓冲接口
// 录制顺序：begin → [barrier] → beginRenderPass → bind* → draw → endRenderPass → end
class ICommandBuffer {
public:
    virtual ~ICommandBuffer() = default;

    // === 生命周期 ===
    virtual void begin() = 0;
    virtual void end() = 0;

    // === 资源屏障（Vulkan 核心，OpenGL 大多空实现） ===
    // 全局内存屏障（Buffer / 通用同步）
    virtual void barrier(PipelineStage srcStage, AccessFlags srcAccess,
                         PipelineStage dstStage, AccessFlags dstAccess) = 0;

    // 纹理屏障（Image Layout 转换 + 访问同步）
    virtual void barrier(TextureHandle texture,
                         PipelineStage srcStage, AccessFlags srcAccess,
                         PipelineStage dstStage, AccessFlags dstAccess,
                         ImageLayout oldLayout, ImageLayout newLayout) = 0;

    // === RenderPass ===
    virtual void beginRenderPass(RenderPassHandle renderPass, FramebufferHandle framebuffer,
                                  const Rect2D& renderArea, std::span<const ClearValue> clearValues) = 0;
    virtual void endRenderPass() = 0;

    // === 管线绑定 ===
    virtual void bindPipeline(PipelineHandle pipeline) = 0;
    virtual void bindGeometry(GeometryHandle geometry) = 0;

    // === 描述符绑定（简化槽位模型） ===
    // 上层无需关心 DescriptorSet / Layout，按 slot 绑定即可
    // Vulkan 后端内部可能使用 push descriptor 或 per-draw 分配
    virtual void bindUniformBuffer(uint32_t slot, BufferHandle buffer, uint64_t offset, uint64_t range) = 0;
    virtual void bindStorageBuffer(uint32_t slot, BufferHandle buffer, uint64_t offset, uint64_t range) = 0;
    virtual void bindTexture(uint32_t slot, TextureHandle texture, SamplerHandle sampler) = 0;
    virtual void bindStorageTexture(uint32_t slot, TextureHandle texture) = 0;  // UAV / ImageStore

    // === 推送常量（小数据快速更新，如 model matrix） ===
    // Vulkan: push constant
    // OpenGL: 内部用 glUniform* 或 UBO 偏移模拟
    virtual void pushConstants(std::span<const uint8_t> data, uint32_t offset = 0) = 0;

    // === 绘制 ===
    virtual void draw(uint32_t vertexCount, uint32_t firstVertex, uint32_t instanceCount = 1) = 0;
    virtual void drawIndexed(uint32_t indexCount, uint32_t firstIndex, int32_t vertexOffset = 0,
                             uint32_t instanceCount = 1) = 0;

    // === 计算 ===
    virtual void dispatchCompute(uint32_t groupX, uint32_t groupY, uint32_t groupZ = 1) = 0;

    // === 复制 ===
    virtual void copyBuffer(BufferHandle src, BufferHandle dst, uint64_t srcOffset, uint64_t dstOffset, uint64_t size) = 0;
    virtual void copyBufferToTexture(BufferHandle src, TextureHandle dst, const Offset3D& dstOffset, const Extent3D& dstExtent) = 0;
    virtual void copyTextureToBuffer(TextureHandle src, BufferHandle dst, const Offset3D& srcOffset, const Extent3D& srcExtent) = 0;

    // === 调试 ===
    virtual void beginDebugLabel(const char* label, std::array<float, 4> color = {1,1,1,1}) = 0;
    virtual void endDebugLabel() = 0;
    virtual void insertDebugMarker(const char* marker) = 0;
};

} // namespace rhi
```

---

## 8. IBuffer.hpp

**重要性：★★★★☆** — GPU 缓冲区描述。

```cpp
#pragma once
#include "Core/Types.hpp"
#include "Core/Handle.hpp"

namespace rhi {

struct BufferDesc {
    uint64_t size = 0;                  // 字节大小
    BufferUsage usage = BufferUsage::None;
    MemoryProperty memory = MemoryProperty::DeviceLocal;

    // 是否可被 CPU 映射（如 staging buffer / 动态 UBO）
    // Vulkan: 影响 VMA allocation flags
    // OpenGL: 影响 glBufferStorage flags 或 glBufferData 用法
    bool cpuAccessible = false;
};

// 缓冲区接口（可选暴露，通常通过 Handle + IRenderDevice 操作）
// 若需显式映射/刷新，可扩展此接口
class IBuffer {
public:
    virtual ~IBuffer() = default;
    virtual uint64_t getSize() const = 0;
    virtual void* map(uint64_t offset, uint64_t size) = 0;      // CPU 映射
    virtual void unmap() = 0;
    virtual void flush(uint64_t offset, uint64_t size) = 0;     // CPU → GPU 可见（非 coherent 时）
};

} // namespace rhi
```

---

## 9. ITexture.hpp

**重要性：★★★★☆** — GPU 纹理描述。

```cpp
#pragma once
#include "Core/Types.hpp"
#include "Core/Format.hpp"
#include "Core/Handle.hpp"

namespace rhi {

enum class TextureType {
    Texture1D,
    Texture2D,
    Texture3D,
    TextureCube,
    Texture2DArray,
};

struct TextureDesc {
    TextureType type = TextureType::Texture2D;
    Format format = Format::Unknown;
    Extent3D extent;           // width/height/depth；Cube 的 depth 为 1，layers=6
    uint32_t mipLevels = 1;
    uint32_t arrayLayers = 1;  // Cube = 6，Array = N
    TextureUsage usage = TextureUsage::Sampled;
    MemoryProperty memory = MemoryProperty::DeviceLocal;
    bool cpuAccessible = false;  // 用于 staging / readback
};

// 纹理视图（可选，用于别名/格式转换，最小实现可暂不实现）
struct TextureViewDesc {
    TextureHandle source;
    Format format;          // 可 reinterpret
    uint32_t baseMipLevel = 0;
    uint32_t mipLevelCount = 1;
    uint32_t baseArrayLayer = 0;
    uint32_t arrayLayerCount = 1;
};

} // namespace rhi
```

---

## 10. IShader.hpp

**重要性：★★★★☆** — 着色器模块。

```cpp
#pragma once
#include "Core/Types.hpp"
#include "Core/Handle.hpp"
#include <span>
#include <cstdint>

namespace rhi {

// 着色器字节码
// Vulkan: SPIR-V (std::span<const uint8_t>)
// OpenGL: GLSL 源码字符串（通过 std::vector<uint8_t> 包装 char 数据）
// 后端根据 stage + 字节码前缀自动判断（如 SPIR-V 魔数 0x07230203）
struct ShaderBytecode {
    std::span<const uint8_t> data;
    // 可选：入口函数名（如 "main"）
    const char* entryPoint = "main";
};

// 着色器模块接口（通常仅用于创建，不暴露额外操作）
class IShader {
public:
    virtual ~IShader() = default;
    virtual ShaderStage getStage() const = 0;
};

} // namespace rhi
```

---

## 11. IPipeline.hpp

**重要性：★★★★★** — 图形/计算管线。顶点格式在此定义，Geometry 不携带格式。

```cpp
#pragma once
#include "Core/Handle.hpp"
#include "Core/Format.hpp"
#include "Core/Types.hpp"
#include <vector>

namespace rhi {

// 顶点属性
// 关键：格式在 Pipeline 里，不在 Geometry 里
// 支持多 vertex buffer 流（bufferSlot）
struct VertexAttribute {
    uint32_t location = 0;      // shader location
    Format format = Format::Unknown;
    uint32_t offset = 0;        // 在 vertex buffer 内的偏移
    uint32_t bufferSlot = 0;    // 对应 GeometryDesc::vertexBuffers[bufferSlot]
    uint32_t stride = 0;        // 该 bufferSlot 的顶点步长
};

// 光栅化状态
struct RasterState {
    enum class CullMode { None, Front, Back };
    enum class FrontFace { CounterClockwise, Clockwise };
    enum class PolygonMode { Fill, Line, Point };

    CullMode cullMode = CullMode::Back;
    FrontFace frontFace = FrontFace::CounterClockwise;
    PolygonMode polygonMode = PolygonMode::Fill;
    bool depthClampEnable = false;
    bool scissorEnable = false;  // 若 true，需额外设置 scissor rect
};

// 深度/模板状态
struct DepthStencilState {
    bool depthTestEnable = true;
    bool depthWriteEnable = true;
    enum class CompareOp { Never, Less, Equal, LessEqual, Greater, NotEqual, GreaterEqual, Always };
    CompareOp depthCompareOp = CompareOp::Less;

    bool stencilTestEnable = false;
    // 简化：暂不暴露完整 stencil op，需要时扩展
};

// 颜色混合状态（每 attachment 一个）
struct BlendState {
    bool enable = false;
    enum class BlendFactor { Zero, One, SrcColor, OneMinusSrcColor, SrcAlpha, OneMinusSrcAlpha, DstAlpha, DstColor };
    enum class BlendOp { Add, Subtract, ReverseSubtract, Min, Max };

    BlendFactor srcColorFactor = BlendFactor::One;
    BlendFactor dstColorFactor = BlendFactor::Zero;
    BlendOp colorOp = BlendOp::Add;
    BlendFactor srcAlphaFactor = BlendFactor::One;
    BlendFactor dstAlphaFactor = BlendFactor::Zero;
    BlendOp alphaOp = BlendOp::Add;

    // 颜色写掩码
    bool writeR = true, writeG = true, writeB = true, writeA = true;
};

// 图形管线描述
struct PipelineDesc {
    ShaderHandle vertexShader;
    ShaderHandle fragmentShader;
    ShaderHandle computeShader;  // 若设置，则为计算管线，忽略 VS/FS

    std::vector<VertexAttribute> vertexLayout;

    RasterState rasterState;
    DepthStencilState depthStencilState;
    std::vector<BlendState> blendStates;  // 数量 = color attachment 数量

    // 渲染通道兼容信息（Vulkan 需要，GL 忽略）
    RenderPassHandle renderPass;  // 兼容的 render pass（可选，VK 强烈建议）
    uint32_t subpassIndex = 0;

    // 推送常量大小（Vulkan: push constant range；GL: 模拟 UBO 或 uniform）
    uint32_t pushConstantsSize = 0;
    
    // 纹理绑定声明：slot → 预期用途（采样 / 存储）
    // 后端据此预分配 descriptor layout / uniform location
    struct TextureSlot {
        uint32_t slot = 0;
        enum class Type { Sampled, Storage } type = Type::Sampled;
        ShaderStage stages = ShaderStage::Fragment;  // 哪些阶段可见
    };
    std::vector<TextureSlot> textureSlots;

    // 推送常量 / uniform buffer 的 slot 声明（可选，用于预校验）
    struct BufferSlot {
        uint32_t slot = 0;
        enum class Type { Uniform, Storage, PushConstant } type = Type::Uniform;
        ShaderStage stages = ShaderStage::Vertex | ShaderStage::Fragment;
        uint32_t size = 0;  // push constant / uniform block 大小
    };
    std::vector<BufferSlot> bufferSlots;
};

// 管线接口（通常仅用于创建）
class IPipeline {
public:
    virtual ~IPipeline() = default;
    virtual bool isCompute() const = 0;
};

} // namespace rhi
```

---

## 12. IGeometry.hpp

**重要性：★★★★★** — 几何数据。无顶点格式，纯数据引用。

```cpp
#pragma once
#include "Core/Handle.hpp"
#include "Core/Types.hpp"
#include <array>

namespace rhi {

// 几何数据描述
// 关键：不包含任何顶点格式信息！格式由 PipelineDesc::vertexLayout 决定
// 支持多 vertex buffer 流（如位置+UV 分开，或位置+法线+切线分开）
struct GeometryDesc {
    static constexpr uint32_t MAX_VERTEX_BUFFERS = 8;

    std::array<BufferHandle, MAX_VERTEX_BUFFERS> vertexBuffers;
    uint32_t vertexBufferCount = 0;

    BufferHandle indexBuffer;     // 可选：null handle = 非索引绘制
    IndexType indexType = IndexType::UInt32;

    uint32_t vertexCount = 0;
    uint32_t indexCount = 0;    // 0 = 非索引
    uint32_t firstVertex = 0;
    uint32_t firstIndex = 0;
};

// 几何接口（通常仅用于创建）
class IGeometry {
public:
    virtual ~IGeometry() = default;
    virtual uint32_t getVertexCount() const = 0;
    virtual uint32_t getIndexCount() const = 0;
};

} // namespace rhi
```

---

## 13. IRenderPass.hpp

**重要性：★★★★☆** — 渲染通道描述。Vulkan 核心，OpenGL 用 FBO 模拟。

```cpp
#pragma once
#include "Core/Handle.hpp"
#include "Core/Format.hpp"
#include "Core/Types.hpp"
#include <vector>

namespace rhi {

// Load / Store 操作
enum class LoadOp {
    Load,       // 保留上一帧内容
    Clear,      // 用 clearValue 清除
    DontCare,   // 内容未定义（性能最优）
};

enum class StoreOp {
    Store,      // 保留供后续使用
    DontCare,   // 丢弃（如 depth 在 pass 后不需要）
};

// 附件描述
struct AttachmentDesc {
    Format format = Format::Unknown;
    uint32_t samples = 1;       // MSAA（先实现 1）
    LoadOp loadOp = LoadOp::Clear;
    StoreOp storeOp = StoreOp::Store;
    ImageLayout initialLayout = ImageLayout::Undefined;  // Vulkan 用，GL 忽略
    ImageLayout finalLayout = ImageLayout::ColorAttachment; // Vulkan 用，GL 忽略
    ClearValue clearValue = {}; // loadOp == Clear 时使用
};

// 子通道描述（最小实现先支持单 subpass）
struct SubpassDesc {
    std::vector<uint32_t> colorAttachments;      // 索引到 RenderPassDesc::attachments
    int32_t depthStencilAttachment = -1;           // -1 = 无
    // 输入附件（延迟渲染用，后续扩展）
    std::vector<uint32_t> inputAttachments;
};

// 子通道依赖（Vulkan 需要，用于自动 barrier 推导）
// 最小实现可忽略，由 RenderGraph 手动插入 barrier
struct SubpassDependency {
    uint32_t srcSubpass;        // VK_SUBPASS_EXTERNAL = ~0u
    uint32_t dstSubpass;
    PipelineStage srcStage;
    PipelineStage dstStage;
    AccessFlags srcAccess;
    AccessFlags dstAccess;
};

// 渲染通道描述
struct RenderPassDesc {
    std::vector<AttachmentDesc> colorAttachments;
    AttachmentDesc depthStencilAttachment;  // format = Unknown = 无
    std::vector<SubpassDesc> subpasses;     // 先实现 1 个
    std::vector<SubpassDependency> dependencies; // 可空
};

// Framebuffer 描述
struct FramebufferDesc {
    RenderPassHandle renderPass;  // 兼容的 render pass
    std::vector<TextureHandle> colorAttachments;
    TextureHandle depthStencilAttachment;  // null = 无
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t layers = 1;
};

} // namespace rhi
```

---

## 14. ISampler.hpp

**重要性：★★★☆☆** — 纹理采样器。

```cpp
#pragma once
#include "Core/Handle.hpp"

namespace rhi {

enum class FilterMode {
    Nearest,
    Linear,
};

enum class MipMode {
    Nearest,
    Linear,
};

enum class WrapMode {
    Repeat,
    ClampToEdge,
    ClampToBorder,
    MirroredRepeat,
};

struct SamplerDesc {
    FilterMode magFilter = FilterMode::Linear;
    FilterMode minFilter = FilterMode::Linear;
    MipMode mipMode = MipMode::Linear;
    WrapMode wrapU = WrapMode::Repeat;
    WrapMode wrapV = WrapMode::Repeat;
    WrapMode wrapW = WrapMode::Repeat;
    float maxAnisotropy = 1.0f;  // 1 = 关闭各向异性
    // 可选：border color, compare op（阴影贴图用）
};

} // namespace rhi
```

---

## 15. Renderer.hpp

**重要性：★★★★☆** — 总入口。聚合 Device、ResourceManager、RenderGraph，隐藏后端细节。

```cpp
#pragma once
#include "Core/Types.hpp"
#include "IRenderDevice.hpp"
#include <memory>

namespace rhi {

// 前向声明
class ResourceManager;
class RenderGraph;

// 后端类型
enum class BackendType {
    OpenGL,
    Vulkan,
    // DirectX12,
    // Metal,
};

// 渲染器配置
struct RendererConfig {
    BackendType backend = BackendType::Vulkan;
    SurfaceDesc surface;
    bool enableValidation = true;
    bool enableDebugMarkers = true;
    uint32_t framesInFlight = 2;
};

// 渲染器总入口
// 生命周期：应用级
// 职责：
//   1. 根据配置创建对应后端 IRenderDevice
//   2. 持有 ResourceManager（所有 GPU 资源的统一创建入口）
//   3. 持有 RenderGraph（可选，若上层自己管理可不用）
//   4. 管理 FrameContext 的 begin/end 生命周期
class Renderer {
public:
    static std::unique_ptr<Renderer> create(const RendererConfig& config);

    ~Renderer();

    // 禁止拷贝
    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;

    // === 子系统访问 ===
    IRenderDevice& device();
    ResourceManager& resources();
    RenderGraph& graph();  // 若未启用 RenderGraph，可断言失败

    // === 帧生命周期 ===
    // 调用 device().beginFrame()，返回 FrameContext
    // 上层不应直接调用 device().beginFrame()
    FrameContext beginFrame();
    void endFrame();
    bool isFrameInProgress() const;

    // === 窗口事件 ===
    void onResize(uint32_t width, uint32_t height);

    // === 工具 ===
    void waitIdle();
    BackendType getBackendType() const;

    // === 便捷查询 ===
    uint32_t getFrameIndex() const;      // 当前 ring buffer 索引
    uint64_t getFrameCount() const;      // 总帧数
    TextureHandle getBackbuffer() const; // 当前帧的 swapchain image（用于 RenderGraph import）

private:
    Renderer() = default;
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace rhi
```

---

## 附录：后端适配要点速查

| 抽象层概念 | Vulkan 实现 | OpenGL 适配 |
|:---|:---|:---|
| **Handle** | `uint32_t` → `VkBuffer`/`VkImage` 等数组索引 | `uint32_t` → `GLuint` 数组索引 |
| **Format** | 直接映射 `VkFormat` | 映射 `GLenum`（`GL_RGBA8`, `GL_FLOAT` 等） |
| **Buffer** | `VkBuffer` + VMA allocation | `GLuint` + `glBufferStorage`/`glBufferData` |
| **Texture** | `VkImage` + `VkImageView` | `GLuint` + `glTexImage2D`/`glTexStorage2D` |
| **Shader** | `VkShaderModule`（SPIR-V） | `GLuint` shader + program（GLSL） |
| **Pipeline** | `VkPipeline`（PSO 缓存） | `GLuint program` + 状态缓存 |
| **Geometry** | `VkBuffer` 引用，无 VAO | `GLuint VAO`（lazy 创建，按 Pipeline 的 vertexLayout 配置） |
| **RenderPass** | `VkRenderPass` + `VkFramebuffer` | `GLuint FBO` + `glBindFramebuffer` |
| **Barrier** | `vkCmdPipelineBarrier` | 空实现；特定 feedback loop 用 `glTextureBarrier` |
| **Descriptor** | `VkDescriptorSet`（pool 分配或 push descriptor） | `glBindBufferRange` + `glActiveTexture` + `glBindSampler` |
| **FrameContext** | `Fence` + `Semaphore` + `CommandPool` Ring Buffer | 几乎空，仅记录帧序号 |
| **beginFrame** | `vkWaitForFences` → `vkAcquireNextImageKHR` → `vkBeginCommandBuffer` | `glClear`（可选） |
| **endFrame** | `vkEndCommandBuffer` → `vkQueueSubmit` → `vkQueuePresentKHR` | `SwapBuffers` |
| **onResize** | `vkDeviceWaitIdle` → 重建 `SwapchainKHR` + `Framebuffer` | `glViewport` |

---

## 附录：目录结构建议

```
include/rhi/
├── core/
│   ├── Handle.hpp
│   ├── Format.hpp
│   └── Types.hpp
├── ISurface.hpp
├── FrameContext.hpp
├── IRenderDevice.hpp
├── ICommandBuffer.hpp
├── IBuffer.hpp
├── ITexture.hpp
├── IShader.hpp
├── IPipeline.hpp
├── IGeometry.hpp
├── IRenderPass.hpp
├── ISampler.hpp
└── Renderer.hpp

src/rhi/
├── gl/
│   ├── GLDevice.hpp / .cpp
│   ├── GLCommandBuffer.hpp / .cpp
│   ├── GLResourcePools.hpp / .cpp   // VAO 缓存、FBO 模拟
│   └── GLUtils.hpp / .cpp           // Format 转换、枚举映射
├── vk/
│   ├── VKDevice.hpp / .cpp
│   ├── VKCommandBuffer.hpp / .cpp
│   ├── VKFrameContext.hpp / .cpp    // Fence/Semaphore/Pool Ring Buffer
│   ├── VKResourcePools.hpp / .cpp   // DescriptorSet 分配、RenderPass 缓存
│   └── VKUtils.hpp / .cpp           // Format 转换、枚举映射、VK_CHECK 宏
├── ResourceManager.hpp / .cpp
├── RenderGraph.hpp / .cpp           // 可选，后续对接
└── Renderer.cpp
```

---

> 设计完成。按此顺序实现：Handle → Format → Types → FrameContext → IRenderDevice → ICommandBuffer → IBuffer/ITexture/IShader/IPipeline/IGeometry → IRenderPass → ISampler → Renderer。
