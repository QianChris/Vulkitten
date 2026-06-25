#pragma once

#include "rhi/Core/Handle.hpp"
#include "rhi/Core/Types.hpp"

namespace rhi {

// ============================================================
// IShader — query interface for created GPU shaders
// ============================================================

class IShader
{
public:
    virtual ~IShader() = default;
    virtual ShaderStage GetStage() const = 0;
    virtual const char* GetEntryPoint() const = 0;
};

} // namespace rhi
