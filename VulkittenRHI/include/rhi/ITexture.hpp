#pragma once

#include "rhi/Core/Export.hpp"
#include "rhi/Core/Handle.hpp"
#include "rhi/Core/Format.hpp"
#include "rhi/Core/Types.hpp"
#include "rhi/ResourceDescs.hpp"

namespace rhi {

// ============================================================
// ITexture — query interface for created GPU textures
// ============================================================

class RHI_API ITexture
{
public:
    virtual ~ITexture() = default;
    virtual TextureType GetType() const = 0;
    virtual Format GetFormat() const = 0;
    virtual Extent3D GetExtent() const = 0;
    virtual uint32_t GetMipLevels() const = 0;
};

} // namespace rhi
