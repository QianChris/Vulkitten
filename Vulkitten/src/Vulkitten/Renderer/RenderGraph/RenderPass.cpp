#include "vktpch.h"
#include "RenderPass.h"

namespace Vulkitten {

RenderPass& RenderPass::Read(const std::string& res, AccessFlags flags)
{
    inputs.push_back({res, flags});
    return *this;
}

RenderPass& RenderPass::Write(const std::string& res, AccessFlags flags)
{
    outputs.push_back({res, flags});
    return *this;
}

RenderPass& RenderPass::SetExecute(ExecuteFunc func)
{
    onExecute = std::move(func);
    return *this;
}

} // namespace Vulkitten
