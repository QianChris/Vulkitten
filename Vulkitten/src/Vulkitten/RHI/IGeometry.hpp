#pragma once

#include "Vulkitten/RHI/Handle.hpp"
#include <cstdint>

namespace Vulkitten {
namespace rhi {

class IGeometry
{
public:
    virtual ~IGeometry() = default;
    virtual uint32_t GetVertexCount() const = 0;
    virtual uint32_t GetIndexCount() const = 0;
};

} // namespace rhi
} // namespace Vulkitten
