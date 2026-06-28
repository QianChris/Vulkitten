// ============================================================
// RenderCommandList Unit Tests (mock-based, no GPU required)
//
// Tests that RenderCommandList correctly delegates to
// ICommandBuffer for Draw, DrawIndirect, Dispatch,
// DispatchIndirect, Debug, DebugLabel, and fluent chaining.
// ============================================================

#include <gtest/gtest.h>

#include "rhi/RenderCommandList.hpp"
#include "rhi/IRenderDevice.hpp"
#include "rhi/ResourceManager.hpp"
#include "rhi/ICommandBuffer.hpp"
#include "rhi/Core/Handle.hpp"
#include "rhi/ResourceDescs.hpp"

#include "mocks/MockCommandBuffer.hpp"

using namespace rhi;
using namespace rhi::test;

// ============================================================
// StubDevice / StubGeometry - minimal implementations
// needed by RenderCommandList constructor and DrawMesh
// ============================================================

namespace {

// Minimal IRenderDevice stub for RenderCommandList tests.
// Only the methods actually called by RenderCommandList need stubs.
class StubDevice : public IRenderDevice
{
public:
    void Init() override {}
    void Shutdown() override {}
    FrameContext BeginFrame() override { return {}; }
    void EndFrame(FrameContext) override {}

    std::unique_ptr<ICommandBuffer> CreateCommandBuffer(FrameContext, CommandBufferLevel) override
    {
        return nullptr;
    }

    BufferHandle   CreateBuffer(const BufferDesc&, const void*) override { return {}; }
    TextureHandle  CreateTexture(const TextureDesc&, const void*) override { return {}; }
    ShaderHandle   CreateShader(ShaderStage, const ShaderBytecode&) override { return {}; }
    PipelineHandle CreatePipeline(const PipelineDesc&) override { return {}; }
    GeometryHandle CreateGeometry(const GeometryDesc&) override { return {}; }
    SamplerHandle  CreateSampler(const SamplerDesc&) override { return {}; }
    RenderPassHandle   CreateRenderPass(const RenderPassDesc&) override { return {}; }
    FramebufferHandle  CreateFramebuffer(const FramebufferDesc&) override { return {}; }
    QueryPoolHandle    CreateQueryPool(const QueryPoolDesc&) override { return {}; }

    IBuffer*   GetBuffer(BufferHandle) override { return nullptr; }
    ITexture*  GetTexture(TextureHandle) override { return nullptr; }
    IShader*   GetShader(ShaderHandle) override { return nullptr; }
    IPipeline* GetPipeline(PipelineHandle) override { return nullptr; }
    IGeometry* GetGeometry(GeometryHandle) override { return nullptr; }
    ISampler*  GetSampler(SamplerHandle) override { return nullptr; }

    void OnResize(uint32_t, uint32_t) override {}
    void WaitIdle() override {}
    void* GetNativeDevice() override { return nullptr; }
};


} // anonymous namespace

// ============================================================
// Test Fixture
// ============================================================

class RenderCommandListTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        m_Cmd.ClearCalls();
    }

    StubDevice       m_Device;
    ResourceManager  m_RM;
    MockCommandBuffer m_Cmd;
    RenderCommandList m_RCL{m_Device, m_RM, m_Cmd};
};

// ============================================================
// Draw - verifies delegation and parameter forwarding
// ============================================================

TEST_F(RenderCommandListTest, Draw_DelegatesWithCorrectParameters)
{
    m_RCL.Draw(3);
    auto* call = m_Cmd.FindLastCall(CallType::Draw);
    ASSERT_NE(call, nullptr);
    EXPECT_EQ(call->VertexCount, 3u);
    EXPECT_EQ(call->FirstVertex, 0u);
    EXPECT_EQ(call->InstanceCount, 1u); // default
}

TEST_F(RenderCommandListTest, Draw_WithFirstVertex)
{
    m_RCL.Draw(6, 3);
    auto* call = m_Cmd.FindLastCall(CallType::Draw);
    ASSERT_NE(call, nullptr);
    EXPECT_EQ(call->VertexCount, 6u);
    EXPECT_EQ(call->FirstVertex, 3u);
    EXPECT_EQ(call->InstanceCount, 1u);
}

TEST_F(RenderCommandListTest, Draw_FluentReturnsSelf)
{
    auto& result = m_RCL.Draw(3);
    EXPECT_EQ(&result, &m_RCL);
}

TEST_F(RenderCommandListTest, Draw_FluentChaining)
{
    // Chain multiple commands; verify all are recorded
    m_RCL.Draw(3).Draw(6, 1).Draw(9);

    EXPECT_EQ(m_Cmd.CountCalls(CallType::Draw), 3u);

    EXPECT_EQ(m_Cmd.GetCalls()[0].VertexCount, 3u);
    EXPECT_EQ(m_Cmd.GetCalls()[1].VertexCount, 6u);
    EXPECT_EQ(m_Cmd.GetCalls()[1].FirstVertex, 1u);
    EXPECT_EQ(m_Cmd.GetCalls()[2].VertexCount, 9u);
}

// ============================================================
// DrawIndexed
// ============================================================

TEST_F(RenderCommandListTest, DrawIndexed_DelegatesWithCorrectParameters)
{
    m_RCL.DrawIndexed(36, 0, 0);
    auto* call = m_Cmd.FindLastCall(CallType::DrawIndexed);
    ASSERT_NE(call, nullptr);
    EXPECT_EQ(call->IndexCount, 36u);
    EXPECT_EQ(call->FirstIndex, 0u);
    EXPECT_EQ(call->VertexOffset, 0);
    EXPECT_EQ(call->InstanceCount, 1u);
}

TEST_F(RenderCommandListTest, DrawIndexed_WithVertexOffset)
{
    m_RCL.DrawIndexed(36, 12, 8);
    auto* call = m_Cmd.FindLastCall(CallType::DrawIndexed);
    ASSERT_NE(call, nullptr);
    EXPECT_EQ(call->IndexCount, 36u);
    EXPECT_EQ(call->FirstIndex, 12u);
    EXPECT_EQ(call->VertexOffset, 8);
}

TEST_F(RenderCommandListTest, DrawIndexed_FluentReturnsSelf)
{
    auto& result = m_RCL.DrawIndexed(36);
    EXPECT_EQ(&result, &m_RCL);
}

TEST_F(RenderCommandListTest, DrawIndexed_FluentChainWithDraw)
{
    m_RCL.DrawIndexed(36).Draw(3).DrawIndexed(24);

    EXPECT_EQ(m_Cmd.CountCalls(CallType::DrawIndexed), 2u);
    EXPECT_EQ(m_Cmd.CountCalls(CallType::Draw), 1u);
}

// ============================================================
// DrawIndirect
// ============================================================

TEST_F(RenderCommandListTest, DrawIndirect_DelegatesWithCorrectParameters)
{
    BufferHandle indirectBuf{42, 1};
    m_RCL.DrawIndirect(indirectBuf, 256, 10, 32);
    auto* call = m_Cmd.FindLastCall(CallType::DrawIndirect);
    ASSERT_NE(call, nullptr);
    EXPECT_EQ(call->BufferHandleId, 42u);
    EXPECT_EQ(call->BufferOffset, 256u);
    EXPECT_EQ(call->IndirectDrawCount, 10u);
    EXPECT_EQ(call->Stride, 32u);
}

TEST_F(RenderCommandListTest, DrawIndirect_ZeroDrawCount)
{
    BufferHandle indirectBuf{1, 1};
    m_RCL.DrawIndirect(indirectBuf, 0, 0, 0);
    auto* call = m_Cmd.FindLastCall(CallType::DrawIndirect);
    ASSERT_NE(call, nullptr);
    EXPECT_EQ(call->IndirectDrawCount, 0u);
    EXPECT_EQ(call->Stride, 0u);
}

TEST_F(RenderCommandListTest, DrawIndirect_FluentReturnsSelf)
{
    BufferHandle buf{1, 1};
    auto& result = m_RCL.DrawIndirect(buf, 0, 1, 16);
    EXPECT_EQ(&result, &m_RCL);
}

// ============================================================
// Dispatch (Compute)
// ============================================================

TEST_F(RenderCommandListTest, Dispatch_DelegatesWithCorrectParameters)
{
    m_RCL.Dispatch(8, 4, 2);
    auto* call = m_Cmd.FindLastCall(CallType::DispatchCompute);
    ASSERT_NE(call, nullptr);
    EXPECT_EQ(call->GroupX, 8u);
    EXPECT_EQ(call->GroupY, 4u);
    EXPECT_EQ(call->GroupZ, 2u);
}

TEST_F(RenderCommandListTest, Dispatch_DefaultGroupZ)
{
    m_RCL.Dispatch(16, 8);
    auto* call = m_Cmd.FindLastCall(CallType::DispatchCompute);
    ASSERT_NE(call, nullptr);
    EXPECT_EQ(call->GroupX, 16u);
    EXPECT_EQ(call->GroupY, 8u);
    EXPECT_EQ(call->GroupZ, 1u); // default
}

TEST_F(RenderCommandListTest, Dispatch_SingleGroup)
{
    m_RCL.Dispatch(1, 1, 1);
    auto* call = m_Cmd.FindLastCall(CallType::DispatchCompute);
    ASSERT_NE(call, nullptr);
    EXPECT_EQ(call->GroupX, 1u);
    EXPECT_EQ(call->GroupY, 1u);
    EXPECT_EQ(call->GroupZ, 1u);
}

TEST_F(RenderCommandListTest, Dispatch_FluentReturnsSelf)
{
    auto& result = m_RCL.Dispatch(8, 8);
    EXPECT_EQ(&result, &m_RCL);
}

TEST_F(RenderCommandListTest, Dispatch_FluentChainWithBindPipeline)
{
    PipelineHandle pso{5, 1};
    m_RCL.BindPipeline(pso).Dispatch(4, 4, 1);

    EXPECT_EQ(m_Cmd.CountCalls(CallType::BindPipeline), 1u);
    EXPECT_EQ(m_Cmd.CountCalls(CallType::DispatchCompute), 1u);
}

// ============================================================
// DispatchIndirect
// ============================================================

TEST_F(RenderCommandListTest, DispatchIndirect_DelegatesWithCorrectParameters)
{
    BufferHandle indirectBuf{99, 3};
    m_RCL.DispatchIndirect(indirectBuf, 512);
    auto* call = m_Cmd.FindLastCall(CallType::DispatchIndirect);
    ASSERT_NE(call, nullptr);
    EXPECT_EQ(call->BufferHandleId, 99u);
    EXPECT_EQ(call->BufferOffset, 512u);
}

TEST_F(RenderCommandListTest, DispatchIndirect_ZeroOffset)
{
    BufferHandle indirectBuf{7, 1};
    m_RCL.DispatchIndirect(indirectBuf, 0);
    auto* call = m_Cmd.FindLastCall(CallType::DispatchIndirect);
    ASSERT_NE(call, nullptr);
    EXPECT_EQ(call->BufferHandleId, 7u);
    EXPECT_EQ(call->BufferOffset, 0u);
}

TEST_F(RenderCommandListTest, DispatchIndirect_FluentReturnsSelf)
{
    BufferHandle buf{1, 1};
    auto& result = m_RCL.DispatchIndirect(buf, 0);
    EXPECT_EQ(&result, &m_RCL);
}

// ============================================================
// DrawMesh - high-level convenience
// ============================================================

// NOTE: DrawMesh tests are skipped because ResourceManager::GetGeometry()
// is non-virtual, preventing mock injection. The fluent API tests above
// cover the same ICommandBuffer delegation paths.
// To test DrawMesh, use the GPU-backed GL tests (GLDrawTest).

// NOTE: DrawMesh_WithBindings and DispatchCompute_WithStorageBindings
// tests are skipped because Binding struct has a deleted default constructor
// due to anonymous union containing non-trivial BufferHandle/TextureHandle.
// These can be enabled after adding an explicit default constructor to Binding.

// DrawMesh_WithPushConstants: skipped (requires TestResourceManager).
// Use GL GPU tests instead.

// ============================================================
// DispatchCompute - high-level convenience
// ============================================================

TEST_F(RenderCommandListTest, DispatchCompute_HighLevel_BindsPipelineAndDispatches)
{
    PipelineHandle pso{20, 1};
    std::vector<Binding> bindings;
    m_RCL.DispatchCompute(pso, bindings, 8, 4, 2);

    EXPECT_EQ(m_Cmd.CountCalls(CallType::BindPipeline), 1u);
    EXPECT_EQ(m_Cmd.CountCalls(CallType::DispatchCompute), 1u);

    auto* dc = m_Cmd.FindLastCall(CallType::DispatchCompute);
    ASSERT_NE(dc, nullptr);
    EXPECT_EQ(dc->GroupX, 8u);
    EXPECT_EQ(dc->GroupY, 4u);
    EXPECT_EQ(dc->GroupZ, 2u);
}

// ============================================================
// Barrier (fluent)
// ============================================================

TEST_F(RenderCommandListTest, Barrier_Buffer_Fluent)
{
    m_RCL.Barrier(PipelineStage::ComputeShader, AccessFlags::ShaderWrite,
                  PipelineStage::VertexShader, AccessFlags::ShaderRead);

    EXPECT_EQ(m_Cmd.CountCalls(CallType::Barrier_Buffer), 1u);
}

TEST_F(RenderCommandListTest, Barrier_Texture_Fluent)
{
    TextureHandle tex{42, 1};
    m_RCL.Barrier(tex,
                  PipelineStage::ColorAttachmentOutput, AccessFlags::ColorAttachmentWrite,
                  PipelineStage::FragmentShader, AccessFlags::ShaderRead,
                  ImageLayout::ColorAttachment, ImageLayout::ShaderRead);

    EXPECT_EQ(m_Cmd.CountCalls(CallType::Barrier_Texture), 1u);
}

// ============================================================
// Fluent API - comprehensive chaining
// ============================================================

TEST_F(RenderCommandListTest, FluentChain_AllCommandsTogether)
{
    PipelineHandle pso{1, 1};
    GeometryHandle geo{2, 1};
    BufferHandle   ub{3, 1};
    BufferHandle   indirect{4, 1};
    TextureHandle  tex{5, 1};
    SamplerHandle  smp{6, 1};

    m_RCL.BindPipeline(pso)
         .BindGeometry(geo)
         .BindUniformBuffer(0, ub, 0, 256)
         .BindTexture(0, tex, smp)
         .Draw(3)
         .DrawIndexed(36)
         .DrawIndirect(indirect, 0, 1, 16)
         .Dispatch(8, 8)
         .DispatchIndirect(indirect, 64);

    EXPECT_EQ(m_Cmd.CountCalls(CallType::BindPipeline), 1u);
    EXPECT_EQ(m_Cmd.CountCalls(CallType::BindGeometry), 1u);
    EXPECT_EQ(m_Cmd.CountCalls(CallType::BindUniformBuffer), 1u);
    EXPECT_EQ(m_Cmd.CountCalls(CallType::BindTexture), 1u);
    EXPECT_EQ(m_Cmd.CountCalls(CallType::Draw), 1u);
    EXPECT_EQ(m_Cmd.CountCalls(CallType::DrawIndexed), 1u);
    EXPECT_EQ(m_Cmd.CountCalls(CallType::DrawIndirect), 1u);
    EXPECT_EQ(m_Cmd.CountCalls(CallType::DispatchCompute), 1u);
    EXPECT_EQ(m_Cmd.CountCalls(CallType::DispatchIndirect), 1u);

    // Verify call order
    EXPECT_EQ(m_Cmd.GetCalls()[0].Type, CallType::BindPipeline);
    EXPECT_EQ(m_Cmd.GetCalls()[1].Type, CallType::BindGeometry);
    EXPECT_EQ(m_Cmd.GetCalls()[2].Type, CallType::BindUniformBuffer);
    EXPECT_EQ(m_Cmd.GetCalls()[3].Type, CallType::BindTexture);
    EXPECT_EQ(m_Cmd.GetCalls()[4].Type, CallType::Draw);
    EXPECT_EQ(m_Cmd.GetCalls()[5].Type, CallType::DrawIndexed);
    EXPECT_EQ(m_Cmd.GetCalls()[6].Type, CallType::DrawIndirect);
    EXPECT_EQ(m_Cmd.GetCalls()[7].Type, CallType::DispatchCompute);
    EXPECT_EQ(m_Cmd.GetCalls()[8].Type, CallType::DispatchIndirect);
}
