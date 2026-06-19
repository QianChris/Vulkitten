#pragma once

#include "Vulkitten/Renderer/RenderGraph/RenderPass.h"

namespace Vulkitten {

class EndPass : public RenderPass
{
public:
    EndPass();
    ~EndPass() = default;
};

} // namespace Vulkitten
