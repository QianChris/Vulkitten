#include "Buffer.h"

#include "Vulkitten/Renderer/IGpuResourceManager.h"
#include "Vulkitten/Renderer/Backend/OpenGL/OpenGLBuffer.h"

#include "Vulkitten/Perf/Instrumentor.h"

namespace Vulkitten {

Ref<VertexBuffer> VertexBuffer::Create(float* vertices, uint32_t size)
    {
        VKT_PROFILE_FUNCTION();

        Ref<VertexBuffer> result = CreateRef<OpenGLVertexBuffer>(vertices, size);

        if (!result)
            return nullptr;

        auto& resources = IRenderer::Get().GetResourceManager();
        GpuBufferDesc desc;
        desc.Size = size;
        uint64_t handle = resources.CreateBuffer(desc, "VertexBuffer");
        resources.TrackExternalRef(handle, result);

        return result;
    }

    Ref<VertexBuffer> VertexBuffer::Create(uint32_t size)
    {
        VKT_PROFILE_FUNCTION();

        Ref<VertexBuffer> result = CreateRef<OpenGLVertexBuffer>(size);

        if (!result)
            return nullptr;

        auto& resources = IRenderer::Get().GetResourceManager();
        GpuBufferDesc desc;
        desc.Size = size;
        uint64_t handle = resources.CreateBuffer(desc, "VertexBuffer");
        resources.TrackExternalRef(handle, result);

        return result;
    }

    Ref<IndexBuffer> IndexBuffer::Create(uint32_t* indices, uint32_t count)
    {
        VKT_PROFILE_FUNCTION();

        Ref<IndexBuffer> result = CreateRef<OpenGLIndexBuffer>(indices, count);

        if (!result)
            return nullptr;

        auto& resources = IRenderer::Get().GetResourceManager();
        GpuBufferDesc desc;
        desc.Size = count * sizeof(uint32_t);
        uint64_t handle = resources.CreateBuffer(desc, "IndexBuffer");
        resources.TrackExternalRef(handle, result);

        return result;
    }
}