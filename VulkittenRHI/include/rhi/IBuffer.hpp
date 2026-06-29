#pragma once

#include "rhi/Core/Export.hpp"
#include "rhi/Core/Handle.hpp"
#include "rhi/Core/Types.hpp"

#include <cstdint>

namespace rhi {

// ============================================================
// IBuffer — query interface for created GPU buffers
// ============================================================

class RHI_API IBuffer
{
public:
    virtual ~IBuffer() = default;
    virtual uint64_t GetSize() const = 0;
    virtual void* Map(uint64_t offset, uint64_t size) = 0;
    virtual void Unmap() = 0;
    virtual void Flush(uint64_t offset, uint64_t size) = 0;
};

} // namespace rhi
