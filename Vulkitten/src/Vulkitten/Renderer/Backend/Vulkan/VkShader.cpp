#include "vktpch.h"
#include "VkShader.h"

#include "VulkanDevice.h"

namespace Vulkitten {

bool VkShader::Compile(VulkanDevice& /*device*/, const std::string& /*glslSource*/,
                       const std::string& /*entryPoint*/, const std::string& /*stage*/)
{
    // Stub: real implementation uses glslang/shaderc for GLSL→SPIR-V
    // then calls vkCreateShaderModule
    return true;
}

} // namespace Vulkitten
