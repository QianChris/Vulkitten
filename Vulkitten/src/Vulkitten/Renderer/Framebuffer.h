#pragma once

#include "Vulkitten/Core/Core.h"

namespace Vulkitten {

    struct FrameBufferSpecification
    {
        uint32_t Width, Height;
        bool SwapChainTarget = false;
    };

    class VKT_API FrameBuffer
    {
    public:
        virtual ~FrameBuffer() = default;

        virtual void Bind() const = 0;
        virtual void Unbind() const = 0;

        virtual uint32_t GetColorAttachmentRendererID() const = 0;

        static Ref<FrameBuffer> Create(const FrameBufferSpecification& spec);
    };

}