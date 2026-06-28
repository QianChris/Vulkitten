#pragma once
#include <string>
#include <vector>

namespace Vulkitten {

enum class ResourceType {
    Texture2D,
    TextureCube,
    Buffer,
    SwapchainImage  // 特殊：BackBuffer
};

enum class TextureFormat {
    RGBA8, RGBA16F, RGBA32F, 
    Depth24Stencil8, Depth32F,
    R11G11B10F  // HDR 常用
};

struct TextureDesc {
    uint32_t width, height;
    TextureFormat format;
    uint32_t mipLevels = 1;
    uint32_t samples = 1;  // MSAA
    bool isTransient = true;  // true = RenderGraph 管理生命周期，false = 外部导入
};

struct BufferDesc {
    size_t size;
    uint32_t usage;  // 位掩码：Vertex/Index/Uniform/Storage
};

struct RenderGraphResource {
    std::string name;
    ResourceType type;
    union {
        TextureDesc texture;
        BufferDesc buffer;
    } desc;
    
    // 编译后由 RenderGraph 填充，外部只读
    uint32_t physicalIndex = UINT32_MAX;  // 指向实际 GPU 资源的索引
    bool imported = false;  // 外部资源（如 Swapchain、GBuffer 预分配）
};

} // namespace Vulkitten