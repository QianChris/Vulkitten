#pragma once

#include <glm/glm.hpp>
#include <cstdint>
#include <variant>
#include "Vulkitten/Renderer/Resources/ResourceHandle.h"
#include "Vulkitten/Renderer/Texture.h"

namespace Vulkitten {

// 前向声明
class Material;
class Mesh;

// 渲染层：按材质排序的键
using SortKey = uint64_t;

// ========== 具体 Command 类型（POD，无继承）==========

// struct DrawMeshCommand {
//     Handle<Mesh> mesh;
//     Handle<Material> material;
//     glm::mat4 transform{1.0f};
//     uint32_t instanceCount = 1;
//     uint32_t renderLayer = 0;      // Opaque=0, Transparent=1, UI=2
//     SortKey sortKey = 0;          // 编译时填充：高32位=shader hash，低32位=depth
    
//     // 调试用
//     #ifdef VKT_DEBUG
//     const char* debugName = nullptr;
//     #endif
// };

struct DrawQuadCommand {
    glm::vec4 color{1.0f};
    Ref<Texture2D> texture;
    float tilingFactor = 1.0f;
    glm::mat4 transform{1.0f};
};

// struct DrawParticleCommand {
//     Handle<Buffer> particleBuffer;   // SSBO 或 structured buffer
//     uint32_t particleCount;
//     Handle<Material> material;
//     glm::mat4 emitterTransform{1.0f};
//     float pointSize = 1.0f;
// };

// struct DrawFullscreenCommand {
//     Handle<Material> material;       // 全屏 Pass：只需要材质（后处理、Blit）
//     // 无 mesh，无 transform
// };

struct ClearCommand {
    glm::vec4 color{0,0,0,1};
    float depth = 1.0f;
    uint32_t stencil = 0;
    bool clearColor = true;
    bool clearDepth = true;
    bool clearStencil = false;
};

// ========== 统一类型（类型安全的 union）==========
using RenderCommand = std::variant<
    // DrawMeshCommand,
    DrawQuadCommand,
    // DrawParticleCommand,
    // DrawFullscreenCommand,
    ClearCommand
    // Extension：DrawInstancedCommand, DrawIndirectCommand, etc.
>;

// 辅助：获取 Command 的排序键（用于 Pass 内排序）
// struct CommandSortKey {
//     SortKey operator()(const RenderCommand& cmd) const {
//         return std::visit([](auto&& c) -> SortKey {
//             using T = std::decay_t<decltype(c)>;
//             // if constexpr (std::is_same_v<T, DrawMeshCommand>) return c.sortKey;
//             // if constexpr (std::is_same_v<T, DrawParticleCommand>) return 0; // 粒子按深度单独排
//             // if constexpr (std::is_same_v<T, DrawFullscreenCommand>) return 0; // 全屏最后
//             if constexpr (std::is_same_v<T, ClearCommand>) return 0; // Clear 最先
//             return 0;
//         }, cmd);
//     }
// };

// // 辅助：获取 Command 的 Material（用于按材质排序）
// struct CommandMaterial {
//     Handle<Material> operator()(const RenderCommand& cmd) const {
//         return std::visit([](auto&& c) -> Handle<Material> {
//             using T = std::decay_t<decltype(c)>;
//             // if constexpr (std::is_same_v<T, DrawMeshCommand>) return c.material;
//             // if constexpr (std::is_same_v<T, DrawParticleCommand>) return c.material;
//             // if constexpr (std::is_same_v<T, DrawFullscreenCommand>) return c.material;
//             return Handle<Material>{}; // ClearCommand 无材质
//         }, cmd);
//     }
// };

}