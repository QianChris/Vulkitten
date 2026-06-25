#pragma once

#include "rhi/Core/Handle.hpp"

namespace rhi {

// ============================================================
// IPipeline — query interface for created GPU pipelines
// ============================================================

class IPipeline
{
public:
    virtual ~IPipeline() = default;
    virtual bool IsCompute() const = 0;
};

} // namespace rhi
