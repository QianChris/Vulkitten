#pragma once

#include "rhi/IBuffer.hpp"
#include "rhi/ITexture.hpp"
#include "rhi/IShader.hpp"
#include "rhi/IPipeline.hpp"
#include "rhi/IGeometry.hpp"
#include "rhi/ISampler.hpp"
#include "rhi/IRenderDevice.hpp"
#include "rhi/ResourceDescs.hpp"

#include <vulkan/vulkan.h>

#include <vector>
#include <memory>

namespace rhi {

// ============================================================
// VKBufferResource — RAII VkBuffer + VkDeviceMemory + IBuffer
// ============================================================
class VKBufferResource : public IBuffer
{
public:
    VKBufferResource(VkDevice device, const BufferDesc& desc, const void* initialData,
                     uint32_t memoryTypeIndex);
    ~VKBufferResource() override;

    VKBufferResource(const VKBufferResource&) = delete;
    VKBufferResource& operator=(const VKBufferResource&) = delete;

    // IBuffer
    uint64_t GetSize() const override { return m_Size; }
    void*    Map(uint64_t offset, uint64_t size) override;
    void     Unmap() override;
    void     Flush(uint64_t offset, uint64_t size) override;

    // Native access
    VkBuffer     GetVkBuffer() const { return m_Buffer; }
    BufferUsage  GetUsage() const { return m_Usage; }

private:
    VkDevice      m_Device = VK_NULL_HANDLE;
    VkBuffer      m_Buffer = VK_NULL_HANDLE;
    VkDeviceMemory m_Memory = VK_NULL_HANDLE;
    uint64_t      m_Size = 0;
    BufferUsage   m_Usage = BufferUsage::None;
};

// ============================================================
// VKTextureResource — RAII VkImage + VkDeviceMemory + VkImageView
// ============================================================
class VKTextureResource : public ITexture
{
public:
    VKTextureResource(VkDevice device, VkPhysicalDevice physicalDevice,
                      const TextureDesc& desc, const void* initialData);
    ~VKTextureResource() override;

    TextureType GetType() const override;
    Format      GetFormat() const override;
    Extent3D    GetExtent() const override;
    uint32_t    GetMipLevels() const override;

    VkImage     GetVkImage() const { return m_Image; }
    VkImageView GetVkImageView() const { return m_ImageView; }

private:
    uint32_t FindMemoryType(VkPhysicalDevice physDev, uint32_t typeFilter,
                            VkMemoryPropertyFlags properties);
    VkDevice         m_Device = VK_NULL_HANDLE;
    VkImage          m_Image = VK_NULL_HANDLE;
    VkDeviceMemory   m_Memory = VK_NULL_HANDLE;
    VkImageView      m_ImageView = VK_NULL_HANDLE;
    TextureDesc      m_Desc;
};

// ============================================================
// VKShaderResource — RAII VkShaderModule + IShader
// ============================================================
class VKShaderResource : public IShader
{
public:
    VKShaderResource(VkDevice device, ShaderStage stage, const ShaderBytecode& bytecode);
    ~VKShaderResource() override;

    bool IsValid() const { return m_ShaderModule != VK_NULL_HANDLE; }

    // IShader
    ShaderStage GetStage() const override { return m_Stage; }
    const char* GetEntryPoint() const override { return "main"; }

    // Native access
    VkShaderModule GetVkShaderModule() const { return m_ShaderModule; }

private:
    VkDevice       m_Device = VK_NULL_HANDLE;
    VkShaderModule m_ShaderModule = VK_NULL_HANDLE;
    ShaderStage    m_Stage;
};

// ============================================================
// VKPipelineResource — RAII VkPipeline + layout + descriptor layout
// ============================================================
class VKPipelineResource : public IPipeline
{
public:
    VKPipelineResource(VkDevice device,
                       const PipelineDesc& desc,
                       VKShaderResource* vs,
                       VKShaderResource* fs,
                       VKShaderResource* cs,
                       VkRenderPass renderPass);
    ~VKPipelineResource() override;

    VKPipelineResource(const VKPipelineResource&) = delete;
    VKPipelineResource& operator=(const VKPipelineResource&) = delete;

    // IPipeline
    bool IsCompute() const override { return m_IsCompute; }

    // Native access
    VkPipeline       GetVkPipeline() const       { return m_Pipeline; }
    VkPipelineLayout GetVkPipelineLayout() const { return m_PipelineLayout; }
    VkDescriptorSetLayout GetVkDescriptorSetLayout() const { return m_DescriptorSetLayout; }
    const std::vector<VertexAttribute>& GetVertexLayout() const { return m_VertexLayout; }
    const std::vector<BufferSlot>&  GetBufferSlots()  const { return m_BufferSlots; }
    const std::vector<TextureSlot>& GetTextureSlots() const { return m_TextureSlots; }
    uint32_t GetPushConstantsSize() const { return m_PushConstantsSize; }

private:
    VkDevice        m_Device = VK_NULL_HANDLE;
    VkPipeline      m_Pipeline = VK_NULL_HANDLE;
    VkPipelineLayout m_PipelineLayout = VK_NULL_HANDLE;
    VkDescriptorSetLayout m_DescriptorSetLayout = VK_NULL_HANDLE;
    bool m_IsCompute = false;

    std::vector<VertexAttribute> m_VertexLayout;
    std::vector<BufferSlot>  m_BufferSlots;
    std::vector<TextureSlot> m_TextureSlots;
    uint32_t m_PushConstantsSize = 0;
};

// ============================================================
// VKGeometryResource — geometry metadata + IGeometry
// ============================================================
class VKGeometryResource : public IGeometry
{
public:
    explicit VKGeometryResource(const GeometryDesc& desc);

    uint32_t GetVertexCount() const override { return m_Desc.VertexCount; }
    uint32_t GetIndexCount()   const override { return m_Desc.IndexCount; }

    const GeometryDesc& GetDesc() const { return m_Desc; }

private:
    GeometryDesc m_Desc;
};

// ============================================================
// VKSamplerResource — RAII VkSampler + ISampler
// ============================================================
class VKSamplerResource : public ISampler
{
public:
    VKSamplerResource(VkDevice device, const SamplerDesc& desc);
    ~VKSamplerResource() override;
    VkSampler GetVkSampler() const { return m_Sampler; }

private:
    VkDevice  m_Device = VK_NULL_HANDLE;
    VkSampler m_Sampler = VK_NULL_HANDLE;
};

} // namespace rhi
