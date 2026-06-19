#pragma once

#include "Vulkitten/Renderer/RenderGraph/RenderPass.h"

namespace Vulkitten {

class GpuParticlePass : public RenderPass
{
public:
    GpuParticlePass();
    ~GpuParticlePass() = default;
};

} // namespace Vulkitten
