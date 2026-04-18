#include <Vulkitten.h>
#include "imgui.h"

class ExampleLayer : public Vulkitten::Layer
{
public:
    ExampleLayer() : Layer("Example") {}

    void OnUpdate() override
    {
        //VKT_INFO("ExampleLayer::Update");

        if (Vulkitten::Input::IsKeyPressed(VKT_KEY_TAB))
            VKT_TRACE("Tab is pressed.");
    }

    virtual void OnImguiRender() override
    {
        ImGui::Begin("Test");
        ImGui::Text("Hello World");
        ImGui::End();
	}

    void OnEvent(Vulkitten::Event& event) override
    {
        if (event.GetEventType() == Vulkitten::EventType::KeyPressed)
        {
            Vulkitten::KeyPressedEvent& e = (Vulkitten::KeyPressedEvent&)event;
            VKT_TRACE("{0}", (char) e.GetKeyCode());
		}
    }
};

class Sandbox : public Vulkitten::Application {
public:
    Sandbox() {
        PushLayer(new ExampleLayer());
    }
    ~Sandbox() {}
};

Vulkitten::Application* Vulkitten::CreateApplication() {
    return new Sandbox();
}
