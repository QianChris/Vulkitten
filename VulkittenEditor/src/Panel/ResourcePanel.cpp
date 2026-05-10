#include "ResourcePanel.h"

#include "Vulkitten/Scene/Components.h"
#include "Vulkitten/Renderer/Texture.h"

#include "imgui.h"

namespace Vulkitten {

    ResourcePanel::ResourcePanel(const Ref<Scene>& scene)
        : m_Context(scene)
    {
    }

    void ResourcePanel::SetContext(const Ref<Scene>& scene)
    {
        m_Context = scene;
    }

    void ResourcePanel::OnImGuiRender()
    {
        if (!m_Context)
            return;

        CollectTextureResources();

        ImGui::Begin("Resource Panel");

        if (ImGui::CollapsingHeader("Textures", ImGuiTreeNodeFlags_DefaultOpen))
        {
            for (auto& resource : m_TextureResources)
            {
                DrawTextureItem(resource);
            }
        }

        ImGui::End();
    }

    void ResourcePanel::CollectTextureResources()
    {
        m_TextureResources.clear();

        if (!m_Context)
            return;

        auto view = m_Context->GetRegistry().view<SpriteRendererComponent>();
        for (auto entity : view)
        {
            auto& sprite = view.get<SpriteRendererComponent>(entity);
            if (sprite.TexturePath.empty())
                continue;

            auto it = std::find_if(m_TextureResources.begin(), m_TextureResources.end(),
                [&sprite](const TextureResource& res) {
                    return res.Path == sprite.TexturePath;
                });

            if (it == m_TextureResources.end())
            {
                TextureResource resource;
                resource.Path = sprite.TexturePath;
                resource.Texture = sprite.Texture;
                resource.Width = sprite.Texture ? sprite.Texture->GetWidth() : 0;
                resource.Height = sprite.Texture ? sprite.Texture->GetHeight() : 0;
                resource.UsageCount = 1;
                m_TextureResources.push_back(resource);
            }
            else
            {
                it->UsageCount++;
            }
        }
    }

    void ResourcePanel::DrawTextureItem(TextureResource& resource)
    {
        ImGui::Bullet();
        ImGui::Text("%s", resource.Path.c_str());
        //ImGui::SameLine();
        ImGui::Text(" [%ux%u] (x%d)", resource.Width, resource.Height, resource.UsageCount);

        if (resource.Texture && resource.Width > 0 && resource.Height > 0)
        {
            float previewSize = 64.0f;
            ImGui::Indent();
            ImGui::Image((void*)(intptr_t)resource.Texture->GetRendererID(),
                ImVec2(previewSize, previewSize));
            ImGui::Unindent();
        }
    }

}