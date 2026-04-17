#include <Vulkitten.h>

class ExampleLayer : public Vulkitten::Layer
{
public:
    ExampleLayer() : Layer("Example") {}

    void OnUpdate() override
    {
        //VKT_INFO("ExampleLayer::Update");
    }

    void OnEvent(Vulkitten::Event& event) override
    {
        //VKT_TRACE("{0}", event.ToString());
    }
};

class Sandbox : public Vulkitten::Application {
public:
    Sandbox() {
        PushLayer(new ExampleLayer());
		PushOverlay(new Vulkitten::ImGuiLayer());
    }
    ~Sandbox() {}
};

Vulkitten::Application* Vulkitten::CreateApplication() {
    return new Sandbox();
}
