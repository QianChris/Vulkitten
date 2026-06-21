#pragma once

#include "Vulkitten/RHI/Handle.hpp"
#include <cstdint>

namespace Vulkitten {
namespace rhi {

class IBuffer
{
public:
    virtual ~IBuffer() = default;
    virtual uint64_t GetSize() const = 0;
    virtual void* Map(uint64_t offset, uint64_t size) = 0;
    virtual void Unmap() = 0;
    virtual void Flush(uint64_t offset, uint64_t size) = 0;
};

} // namespace rhi
} // namespace Vulkitten
