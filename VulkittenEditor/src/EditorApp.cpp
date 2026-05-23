#include "Vulkitten/Core/Application.h"
#include "Vulkitten/Core/FileSystem.h"

#include <filesystem>
#include "EditorLayer.h"

#include <VulkittenEntry.h>

namespace Vulkitten {
    class EditorApp : public Application {
    public:
        EditorApp() {
            auto currPath = std::filesystem::current_path().string();
            VKT_INFO("Current path is {0}", currPath);

            FileSystem::RegisterPath("../../VulkittenEditor", "editor");
            FileSystem::RegisterPath("../../Sandbox", "sandbox");

            PushLayer(new EditorLayer());
        }
        ~EditorApp() {}
    };
}

Vulkitten::Application* Vulkitten::CreateApplication() {
    return new Vulkitten::EditorApp();
}