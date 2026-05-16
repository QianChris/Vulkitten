#include <Vulkitten.h>
#include <VulkittenEntry.h>

#include <filesystem>

#include "DefaultLayer.h"

class EditorApp : public Vulkitten::Application {
public:
    EditorApp() {
        auto currPath = std::filesystem::current_path().string();
        VKT_INFO("Current path is {0}", currPath);
        
        Vulkitten::FileSystem::RegisterPath("../../VulkittenEditor", "editor");
        Vulkitten::FileSystem::RegisterPath("../../Sandbox", "sandbox");

        PushLayer(new DefaultLayer());
    }
    ~EditorApp() {}
};

Vulkitten::Application* Vulkitten::CreateApplication() {
    return new EditorApp();
}
