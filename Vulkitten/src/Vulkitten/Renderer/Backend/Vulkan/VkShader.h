#pragma once

#include "Vulkitten/Core/Core.h"

#include <string>

namespace Vulkitten {

class VulkanDevice;

// ============================================================
// VkShader — Vulkan shader module (GLSL → SPIR-V).
//
// Compiles GLSL source to SPIR-V at runtime using glslang or
// shaderc, then creates a VkShaderModule for pipeline usage.
// ============================================================
class VkShader
{
public:
    VkShader() = default;
    ~VkShader() = default;

    // Compile GLSL to SPIR-V and create VkShaderModule.
    // Returns true on success.
    bool Compile(VulkanDevice& device, const std::string& glslSource,
                 const std::string& entryPoint, const std::string& stage);

    void* GetNativeShaderModule() const { return m_ShaderModule; }

private:
    void* m_ShaderModule = nullptr; // VkShaderModule
};

} // namespace Vulkitten
