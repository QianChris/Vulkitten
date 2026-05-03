#include <Vulkitten.h>
#include <VulkittenEntry.h>

#include <filesystem>


class EditorApp : public Vulkitten::Application {
public:
    EditorApp() {
        auto currPath = std::filesystem::current_path().string();
        VKT_INFO("Current path is {0}", currPath);
        
        Vulkitten::FileSystem::RegisterPath("../../VulkittenEditor", "editor");

        //PushLayer(new ExampleLayer());
        // PushLayer(new Sandbox2D());
    }
    ~EditorApp() {}
};

Vulkitten::Application* Vulkitten::CreateApplication() {
    return new EditorApp();
}
