#pragma once

#include "rhi/Core/Export.hpp"
#include "rhi/Core/Handle.hpp"

#include <cstdint>

namespace rhi {

// ============================================================
// IGeometry — query interface for created GPU geometry
// ============================================================

class RHI_API IGeometry
{
public:
    virtual ~IGeometry() = default;
    virtual uint32_t GetVertexCount() const = 0;
    virtual uint32_t GetIndexCount() const = 0;
};

} // namespace rhi
