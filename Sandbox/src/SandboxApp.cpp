#include <Vulkitten.h>
#include <VulkittenEntry.h>

#include <filesystem>

//#include "ExampleLayer.h"
//#include "Sandbox2D.h"
#include "ExampleLayer2.h"
//#include "ExampleLayer3D.h"   // Uncomment to test 3D glTF loading

class Sandbox : public Vulkitten::Application {
public:
    Sandbox() {
        // For debugging
        auto currPath = std::filesystem::current_path().string();
        VKT_INFO("Current path is {0}", currPath);
        
        Vulkitten::Engine::Get().GetFileSystem().RegisterPath("../../Sandbox", "sandbox");

        // PushLayer(new ExampleLayer());
        //PushLayer(new Sandbox2D());
        PushLayer(new ExampleLayer2());
    }
    ~Sandbox() {}
};

Vulkitten::Application* Vulkitten::CreateApplication() {
    // Select OpenGL backend (Vulkan is WIP — white screen)
    Vulkitten::Application::SetBackend(Vulkitten::RendererBackend::OpenGL);
    return new Sandbox();
}
