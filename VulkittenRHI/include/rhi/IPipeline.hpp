#pragma once

#include "rhi/Core/Export.hpp"
#include "rhi/Core/Handle.hpp"

namespace rhi {

// ============================================================
// IPipeline — query interface for created GPU pipelines
// ============================================================

class RHI_API IPipeline
{
public:
    virtual ~IPipeline() = default;
    virtual bool IsCompute() const = 0;
};

} // namespace rhi
