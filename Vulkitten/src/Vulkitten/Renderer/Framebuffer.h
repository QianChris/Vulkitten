#pragma once

#include "Vulkitten/Core/Core.h"

#include <glm/glm.hpp>

namespace Vulkitten {

    enum class FramebufferTextureFormat
    {
        None = 0,

        // Color
        RGBA8,
        RED_INTEGER,

        // Depth/stencil
        Depth24Stencil8,

        // Default
        DEPTH = Depth24Stencil8,
    };

    struct FramebufferTextureSpecification
    {
        FramebufferTextureFormat TextureFormat = FramebufferTextureFormat::None;
        // TODO: Filter
        // TODO: Wrap
    };

    struct FramebufferAttachmentSpecification
    {
        FramebufferAttachmentSpecification() = default;
        FramebufferAttachmentSpecification(std::initializer_list<FramebufferTextureSpecification> attachments)
            : Attachments(attachments) {}

        std::vector<FramebufferTextureSpecification> Attachments;
    };

    struct FramebufferSpecification
    {
        uint32_t Width = 1280, Height = 720;
        FramebufferAttachmentSpecification Attachments;
        uint32_t Samples = 1;
        glm::vec4 ClearColor = { 0.0f, 0.0f, 0.0f, 1.0f };

        bool SwapChainTarget = false;
    };

    class VKT_API Framebuffer
    {
    public:
        virtual ~Framebuffer() = default;

        virtual void Bind() const = 0;
        virtual void Unbind() const = 0;

		virtual void Resize(uint32_t width, uint32_t height) = 0;
		virtual int ReadPixel(uint32_t attachmentIndex, int x, int y) = 0;

		virtual void ClearAttachment(uint32_t attachmentIndex, int value) = 0;

		virtual uint32_t GetColorAttachmentRendererID(uint32_t index = 0) const = 0;

		virtual const FramebufferSpecification& GetSpecification() const = 0;

        static Ref<Framebuffer> Create(const FramebufferSpecification& spec);
    };

}