#pragma once

#include "Vulkitten/Renderer/RenderGraph/RenderPass.h"

namespace Vulkitten {

class PreparePass : public RenderPass
{
public:
    PreparePass();
    ~PreparePass() = default;
};

} // namespace Vulkitten
