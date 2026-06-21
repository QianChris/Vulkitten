#pragma once

#include "Vulkitten/RHI/Handle.hpp"
#include "Vulkitten/RHI/Core/Format.hpp"
#include "Vulkitten/RHI/Core/Types.hpp"

namespace Vulkitten {
namespace rhi {

class ITexture
{
public:
    virtual ~ITexture() = default;
    virtual TextureType GetType() const = 0;
    virtual Format GetFormat() const = 0;
    virtual Extent3D GetExtent() const = 0;
    virtual uint32_t GetMipLevels() const = 0;
};

} // namespace rhi
} // namespace Vulkitten
