#pragma once
#include "Vulkitten/Core/Core.h"
#include "Vulkitten/Renderer/Framebuffer.h"

namespace Vulkitten {

    class OpenGLFramebuffer : public Framebuffer
    {
    public:
        OpenGLFramebuffer(const FramebufferSpecification& spec);
        virtual ~OpenGLFramebuffer();

		void Invalidate();
        
        virtual void Bind() const override;
        virtual void Unbind() const override;

		virtual void Resize(uint32_t width, uint32_t height) override;
		virtual int ReadPixel(uint32_t attachmentIndex, int x, int y) override;

		virtual void ClearAttachment(uint32_t attachmentIndex, int value) override;

        virtual uint32_t GetColorAttachmentRendererID(uint32_t index = 0) const override { VKT_CORE_ASSERT(index < m_ColorAttachments.size(), "Index out of bounds!"); return m_ColorAttachments[index]; }
		virtual const FramebufferSpecification& GetSpecification() const override { return m_Specification; }

    private:
        uint32_t m_RendererID = 0;

        FramebufferSpecification m_Specification;

        std::vector<FramebufferTextureSpecification> m_ColorAttachmentSpecifications;
        FramebufferTextureSpecification m_DepthAttachmentSpecification;

        std::vector<uint32_t> m_ColorAttachments;
        uint32_t m_DepthAttachment = 0;
    };

}