#include "vktpch.h"
#include "PreparePass.h"

#include "Vulkitten/Renderer/IRenderer.h"
#include "Vulkitten/Renderer/Device.h"
#include "Vulkitten/Renderer/FrameContext.h"
#include "Vulkitten/RHI/ICommandBuffer.hpp"
#include "Vulkitten/Renderer/Backend/OpenGL/OpenGLRenderer.h"
#include "Vulkitten/Renderer/RenderGraph/RenderGraph.h"
#include "Vulkitten/Renderer/Framebuffer.h"

namespace Vulkitten {

PreparePass::PreparePass()
{
    name = "PreparePass";

    SetExecute([this](const std::vector<RenderGraphResource>& /*resources*/,
                      const std::vector<RenderCommand>& commands,
                      void* /*backendContext*/) {
        // Bind configured Framebuffer
        auto* graph = GetGraph();
        auto fb = graph ? graph->GetFramebuffer("Viewport") : nullptr;
        if (fb)
            fb->Bind();

        // Collect clear values from commands
        rhi::ClearValue clearValues[4];
        uint32_t clearCount = 0;
        for (auto& cmd : commands)
        {
            if (auto* clearCmd = std::get_if<ClearCommand>(&cmd))
            {
                clearValues[clearCount].Color.RGBA = {
                    clearCmd->color.r, clearCmd->color.g,
                    clearCmd->color.b, clearCmd->color.a};
                clearCount++;
                if (clearCount >= 4) break;
            }
        }

        // Record clear via ICommandBuffer
        if (clearCount > 0)
        {
            auto& device = IRenderer::Get().GetDevice();
            auto* cmd = device.createCommandBuffer(FrameContext{});
            if (cmd)
            {
                cmd->Begin();
                rhi::Rect2D area;
                cmd->BeginRenderPass({}, {}, area, clearValues, clearCount);
                cmd->EndRenderPass();
                cmd->End();
                delete cmd;
            }
        }

        if (fb)
            fb->Unbind();
    });
}

} // namespace Vulkitten
