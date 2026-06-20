#pragma once

#include <Vulkitten.h>
#include "imgui.h"

class ExampleLayer2 : public Vulkitten::Layer
{
public:
    ExampleLayer2() : Layer("Empty")
    {
        m_Scene = Vulkitten::Engine::Get().CreateEmptyScene();
        Vulkitten::RenderContext::Get().GetShaderLibrary().Load("engine://shaders/FlatColor.shader");

        using namespace Vulkitten;
        m_Texture = Texture2D::Create("sandbox://assets/textures/Checkerboard.png");

        CreateTestScene();

        m_Scene->AddSystem(CreateScope<RenderSystem>());
    }

    void CreateTestScene()
    {
        using namespace Vulkitten;
        {
            auto entity = m_Scene->CreateEntity("Camera");
            auto& cameraComponent = entity.AddComponent<CameraComponent>();
            cameraComponent.Primary = true;
            cameraComponent.FixedAspectRatio = false;

            auto& window = Vulkitten::Application::Get().GetWindow();
            float aspectRatio = static_cast<float>(window.GetWidth()) / window.GetHeight();
            cameraComponent.Camera.SetOrthographicProjection(-aspectRatio * 1.0f, aspectRatio * 1.0f, -1.0f, 1.0f);
            cameraComponent.Camera.SetOrthographicNear(0.001f);
            cameraComponent.Camera.SetOrthographicFar(1000.f);

            auto& cameraTransform = entity.GetComponent<TransformComponent>();
            cameraTransform.SetPosition({ 0., 0., 1. });
        }

        {
            Entity entity = m_Scene->CreateEntity("Green Quad");
            entity.AddComponent<SpriteRendererComponent>(glm::vec4(0.2f, 0.8f, 0.3f, 1.0f), nullptr, 1.0f);
            auto& transform = entity.GetComponent<TransformComponent>();
            transform.SetPosition({ 0.5f, -0.5f, 0.1f });
            transform.SetScale({ 0.5f, 0.75f, 1.0f });
        }

        {
            Entity entity = m_Scene->CreateEntity("Red Quad");
            entity.AddComponent<SpriteRendererComponent>(glm::vec4(0.8f, 0.2f, 0.3f, 1.0f), nullptr, 1.0f);
            auto& transform = entity.GetComponent<TransformComponent>();
            transform.SetPosition({ -0.5f, 0.0f, 0.1f });
            transform.SetScale({ 0.75f, 0.75f, 1.0f });
        }

        {
            Entity entity = m_Scene->CreateEntity("Background Quad");
            entity.AddComponent<SpriteRendererComponent>(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f), m_Texture, 10.0f);
            auto& transform = entity.GetComponent<TransformComponent>();
            transform.SetPosition({ 0.0f, 0.0f, 0.0f });
            transform.SetScale({ 10.0f, 10.0f, 1.0f });
        }
    }

    void OnUpdate(Vulkitten::Timestep timestep) override
    {
        // Clear is now handled by RenderSystem → RenderGraph PreparePass
        m_Scene->SetEditorCamera(nullptr);
        m_Scene->OnUpdate(timestep);
    }

    virtual void OnImguiRender() override
    {
	}

    void OnEvent(Vulkitten::Event& event) override
    {
        Vulkitten::EventDispatcher dispatcher(event);

        // Window resize: update camera aspect ratio to prevent stretching
        dispatcher.Dispatch<Vulkitten::WindowResizeEvent>([this](Vulkitten::WindowResizeEvent& e)
        {
            float aspect = static_cast<float>(e.GetWidth()) / static_cast<float>(e.GetHeight());
            m_Scene->SetCameraAspectRatio(aspect);
            return false;
        });

        // OnEvent pattern: modify Component data only.
        dispatcher.Dispatch<Vulkitten::KeyPressedEvent>([this](Vulkitten::KeyPressedEvent& e)
        {
            if (e.GetKeyCode() == VKT_KEY_SPACE)
            {
                auto view = m_Scene->GetRegistry().view<Vulkitten::SpriteRendererComponent>();
                for (auto entity : view)
                {
                    auto& sprite = view.get<Vulkitten::SpriteRendererComponent>(entity);
                    sprite.Color = { 1.0f, 0.0f, 0.0f, 1.0f };
                }
                return true;
            }
            return false;
        });
    }

private:
    Vulkitten::Scope<Vulkitten::Scene> m_Scene;
    Vulkitten::Ref<Vulkitten::Texture2D> m_Texture;
};