#pragma once

#include "Vulkitten/Core/Core.h"
#include "Vulkitten/Scene/Scene.h"
#include "Vulkitten/Scene/Entity.h"

#include <string>
#include <vector>
#include <utility>

namespace Vulkitten {

    struct TextureResource {
        std::string Path;
        Ref<Texture2D> Texture;
        uint32_t Width;
        uint32_t Height;
        uint32_t UsageCount = 0;
    };

    class ResourcePanel
    {
    public:
        ResourcePanel() = default;
        ResourcePanel(const Ref<Scene>& scene);

        void SetContext(const Ref<Scene>& scene);
        void OnImGuiRender();

    private:
        void CollectTextureResources();
        void DrawTextureItem(TextureResource& resource);

    private:
        Ref<Scene> m_Context;
        std::vector<TextureResource> m_TextureResources;
    };

}