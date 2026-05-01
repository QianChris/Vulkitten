#include <Vulkitten.h>
#include <VulkittenEntry.h>

#include <filesystem>

#include "ExampleLayer.h"
#include "Sandbox2D.h"

class Sandbox : public Vulkitten::Application {
public:
    Sandbox() {
        auto currPath = std::filesystem::current_path().string();
        VKT_INFO("Current path is {0}", currPath);
        
        Vulkitten::FileSystem::RegisterPath("../../Sandbox", "sandbox");

        //PushLayer(new ExampleLayer());
        PushLayer(new Sandbox2D());
    }
    ~Sandbox() {}
};

Vulkitten::Application* Vulkitten::CreateApplication() {
    return new Sandbox();
}
