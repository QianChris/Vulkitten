#pragma once 

#include "Vulkitten/Core/Core.h"

#include <vector>
#include <queue>
#include <array>
#include <unordered_map>
#include <string>
#include <cstdint>
#include <type_traits>

namespace Vulkitten {

    template<typename T>
    struct Handle;

    class Buffer;
    class Texture;
    class Framebuffer;

    class BufferTag {};
    class TextureTag {};
    class FramebufferTag {};

    // 内部存储槽（类型擦除，用 union 或 variant）
    struct ResourceSlot {
        enum class Type { None, Buffer, Texture, Framebuffer } type = Type::None;
        
        union ResourceUnion {
            // 这些在实际实现中替换为你的平台类型
            // 这里用 void* 占位，实际应该是 VkBuffer, GLuint 等
            uint64_t buffer;      // 平台相关的 buffer handle
            uint64_t texture;     // 平台相关的 texture handle  
            uint64_t framebuffer; // 平台相关的 framebuffer handle
        } resource;
        
        uint16_t generation = 0;
        bool alive = false;
        uint32_t lastUsedFrame = 0;      // 最后使用的帧索引
        size_t size = 0;                 // 内存大小（用于统计和复用）
        
        // 描述信息（用于重建/调试）
        std::string debugName;
    };

    class VKT_API ResourcePool
    {
    public:
        ResourcePool();
        ~ResourcePool();

        // 资源管理接口
        Handle<Buffer> CreateBuffer(const std::string& name);
        Handle<Texture> CreateTexture(const std::string& name);
        Handle<Framebuffer> CreateFramebuffer(const std::string& name);

        // 获取资源
        Buffer* GetBuffer(const Handle<Buffer>& handle);
        Texture* GetTexture(const Handle<Texture>& handle);
        Framebuffer* GetFramebuffer(const Handle<Framebuffer>& handle);

        // 销毁资源
        void DestroyBuffer(const Handle<Buffer>& handle);
        void DestroyTexture(const Handle<Texture>& handle);
        void DestroyFramebuffer(const Handle<Framebuffer>& handle);

        // 获取资源描述（用于 RenderGraph 的 transient resource 复用）
        const ResourceSlot* GetSlot(uint32_t index) const;
        
        // 强制立即销毁（仅用于 Shutdown 时的清理）
        void ForceDestroyAll();
        
    private:
        std::vector<ResourceSlot> m_Slots;
        std::queue<uint32_t> m_FreeIndices;  // 回收的槽位
        
        // 延迟释放队列：按帧分桶
        // m_PendingDeletion[frameIndex] = 该帧标记删除的资源索引列表
        std::array<std::vector<uint32_t>, MAX_FRAMES_IN_FLIGHT> m_PendingDeletion;
        
        uint32_t AllocateSlot();
        void DestroySlot(uint32_t index);  // 真正调用 vkDestroy / glDelete
        
        // 防 ABA：generation 检查
        bool ValidateHandle(uint32_t index, uint16_t generation) const;
    };

} // namespace Vulkitten