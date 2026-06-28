#pragma once

#include "Vulkitten/RHI/Handle.hpp"

namespace Vulkitten {
namespace rhi {

class IPipeline
{
public:
    virtual ~IPipeline() = default;
    virtual bool IsCompute() const = 0;
};

} // namespace rhi
} // namespace Vulkitten
