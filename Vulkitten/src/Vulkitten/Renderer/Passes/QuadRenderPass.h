#pragma once 

#include "Vulkitten/Renderer/RenderGraph/RenderPass.h"

namespace Vulkitten {

class QuadRenderPass : public RenderPass {
public:
    QuadRenderPass();
    ~QuadRenderPass() = default;
};

} // namespace Vulkitten