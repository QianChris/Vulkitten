#include "vktpch.h"
#include "Texture.h"

#include "Vulkitten/Renderer/Renderer.h"
#include "Vulkitten/Renderer/IRenderer.h"
#include "Vulkitten/Renderer/IGpuResourceManager.h"
#include "Vulkitten/Renderer/Backend/OpenGL/OpenGLTexture.h"

#include "Vulkitten/Perf/Instrumentor.h"

namespace Vulkitten {

    Ref<Texture2D> Texture2D::Create(uint32_t width, uint32_t height)
    {
        VKT_PROFILE_FUNCTION();

        Ref<Texture2D> result;
        switch (Renderer::GetAPI())
        {
            case RendererAPI::API::None:
                VKT_CORE_ASSERT(false, "RendererAPI::None is currently not supported!");
                return nullptr;
            case RendererAPI::API::OpenGL:
                result = CreateRef<OpenGLTexture2D>(width, height);
                break;
        }

        if (!result)
            return nullptr;

        // Register with GpuResourceManager for lifecycle tracking.
        auto& resources = IRenderer::Get().GetResourceManager();
        GpuTextureDesc desc;
        desc.Width = result->GetWidth();
        desc.Height = result->GetHeight();
        uint64_t handle = resources.CreateTexture(desc, "Texture2D");
        resources.SetGpuHandle(handle, result->GetRendererID());
        resources.TrackExternalRef(handle, result);

        return result;
    }

    Ref<Texture2D> Texture2D::Create(const std::string& path)
    {
        VKT_PROFILE_FUNCTION();

        Ref<Texture2D> result;
        switch (Renderer::GetAPI())
        {
            case RendererAPI::API::None:
                VKT_CORE_ASSERT(false, "RendererAPI::None is currently not supported!");
                return nullptr;
            case RendererAPI::API::OpenGL:
                result = CreateRef<OpenGLTexture2D>(path);
                break;
        }

        if (!result)
            return nullptr;

        // Register with GpuResourceManager for lifecycle tracking.
        auto& resources = IRenderer::Get().GetResourceManager();
        GpuTextureDesc desc;
        desc.Width = result->GetWidth();
        desc.Height = result->GetHeight();
        uint64_t handle = resources.CreateTexture(desc, path);
        resources.SetGpuHandle(handle, result->GetRendererID());
        resources.TrackExternalRef(handle, result);

        return result;
    }

}