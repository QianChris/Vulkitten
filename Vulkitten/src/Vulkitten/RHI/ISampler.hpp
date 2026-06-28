#pragma once

#include "Vulkitten/RHI/Handle.hpp"

namespace Vulkitten {
namespace rhi {

// Sampler is mostly a tag - all configuration is in SamplerDesc.
// This interface exists for type completeness and future expansion.
class ISampler
{
public:
    virtual ~ISampler() = default;
};

} // namespace rhi
} // namespace Vulkitten
