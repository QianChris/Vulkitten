#pragma once
#include <vector>
#include <functional>

#include "RenderCommand.h"
#include "RenderGraphResource.h"

namespace Vulkitten {

// 资源访问标志
enum class AccessFlags {
    None = 0,
    Read = 1 << 0,
    Write = 1 << 1,
    ReadWrite = Read | Write
};

struct PassResourceUsage {
    std::string resourceName;
    AccessFlags access;
    // 管线阶段（用于屏障推断，Vulkan 必需，OpenGL 可忽略）
    uint32_t pipelineStages = 0;  
    uint32_t imageLayout = 0;  // Vulkan 专用，OpenGL 忽略
};

class RenderPass {
public:
    std::string name;
    uint32_t index = 0;  // 编译后填充
    
    // 声明阶段：告诉图"我要读/写哪些资源"
    std::vector<PassResourceUsage> inputs;
    std::vector<PassResourceUsage> outputs;
    
    // 是否写入 BackBuffer（影响 Present 同步）
    bool writesToSwapchain = false;
    
    // 执行回调：由 RenderGraph 在合适的时机调用
    // 参数：resourcePool 是编译后的实际 GPU 资源数组
    using ExecuteFunc = std::function<void(
        const std::vector<RenderGraphResource>& resourcePool,
        const std::vector<RenderCommand>& commands,  // 该 Pass 过滤后的命令
        void* backendContext  // 实际为 VkCommandBuffer / 无（OpenGL 全局状态）
    )>;
    
    ExecuteFunc onExecute;
    
    // 辅助：快速声明
    RenderPass& Read(const std::string& res, AccessFlags flags = AccessFlags::Read);
    RenderPass& Write(const std::string& res, AccessFlags flags = AccessFlags::Write);
    RenderPass& SetExecute(ExecuteFunc func);
};

} // namespace Vulkitten