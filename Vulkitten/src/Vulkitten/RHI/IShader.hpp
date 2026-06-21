#pragma once

#include "Vulkitten/RHI/Handle.hpp"
#include "Vulkitten/RHI/Core/Types.hpp"

namespace Vulkitten {
namespace rhi {

class IShader
{
public:
    virtual ~IShader() = default;
    virtual ShaderStage GetStage() const = 0;
    virtual const char* GetEntryPoint() const = 0;
};

} // namespace rhi
} // namespace Vulkitten
