#pragma once
#include "Vulkitten/Core/Core.h"
#include "Vulkitten/Renderer/FrameBuffer.h"

namespace Vulkitten {

    class OpenGLFrameBuffer : public FrameBuffer
    {
    public:
        OpenGLFrameBuffer(const FrameBufferSpecification& spec);
        virtual ~OpenGLFrameBuffer();

        virtual void Bind() const override;
        virtual void Unbind() const override;

        virtual uint32_t GetColorAttachmentRendererID() const override { return m_ColorAttachment; }

    private:
        uint32_t m_RendererID = 0;
        uint32_t m_ColorAttachment = 0;

        FrameBufferSpecification m_Specification;
    };

}