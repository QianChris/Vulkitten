#pragma once

#include "rhi/Core/Handle.hpp"

namespace rhi {

// ============================================================
// ISampler — query interface for created GPU samplers
//
// Mostly a type tag; all configuration is in SamplerDesc.
// ============================================================

class ISampler
{
public:
    virtual ~ISampler() = default;
};

} // namespace rhi
