#include "vktpch.h"
#include "Engine.h"

#include "Vulkitten/Core/ClassFactory.h"
#include "Vulkitten/Core/FileSystem.h"
#include "Vulkitten/Core/Input.h"
#include "Vulkitten/Core/Log.h"
#include "Vulkitten/Scene/Scene.h"
#include "Vulkitten/Scene/Entity.h"
#include "Vulkitten/Perf/Instrumentor.h"
#include <unordered_set>

namespace Vulkitten {

Engine::Engine()
    : m_FileSystem(CreateScope<FileSystem>())
{
}

Engine::~Engine() = default;

void Engine::Init()
{
    VKT_PROFILE_FUNCTION();

    VKT_CORE_INFO("Engine::Init - engine subsystems initializing...");

    // FileSystem: virtual paths are currently registered in Application::Application().
    // In a future task, RegisterPath calls will move here.
    // For now, the FileSystem instance is ready but empty — paths are added externally.

    // Input: currently initialized as a global static (WindowsInput.cpp line 10).
    // Engine owns the lifecycle: Shutdown() will clean it up.
    // Future: Engine::Init() will create the platform Input implementation and
    // set Input::s_Instance, removing the global static initializer.

    VKT_CORE_INFO("Engine::Init - complete.");
}

void Engine::Shutdown()
{
    VKT_PROFILE_FUNCTION();

    VKT_CORE_INFO("Engine::Shutdown — shutting down...");

    // Input lifecycle: delete the platform Input singleton if it exists.
    // (Currently created as a global static; Engine takes ownership of cleanup.)
    // Future: Input creation moves into Init(), making this symmetrical.
    // delete Input::s_Instance; Input::s_Instance = nullptr;  // uncomment when Engine owns Input creation

    // FileSystem cleanup: clear virtual path mappings.
    // (Will be relevant when RegisterPath moves to Engine::Init)

    VKT_CORE_INFO("Engine::Shutdown — complete.");
}

Log& Engine::GetLogger()
{
    return Log::Get();
}

Engine& Engine::Get()
{
    return ClassFactory::GetInstance<Engine>();
}

// ---- Scene Factory ----

Scope<Scene> Engine::CreateEmptyScene()
{
    return CreateScope<Scene>();
}

Scope<Scene> Engine::LoadSceneFromGltf(const std::string& filepath)
{
    VKT_CORE_WARN("Engine::LoadSceneFromGltf — stub, glTF loading not yet implemented ({0})", filepath);
    return CreateScope<Scene>();
}

void Engine::MergeScenes(Scene& target, Scene& source)
{
    auto& srcRegistry = source.GetRegistry();
    auto& dstRegistry = target.GetRegistry();

    // Collect all entity handles from source by iterating known component views.
    std::unordered_set<entt::entity> seen;

    auto collect = [&](auto& view) {
        for (auto entity : view)
            seen.insert(entity);
    };

    collect(srcRegistry.view<TagComponent>());
    collect(srcRegistry.view<TransformComponent>());
    collect(srcRegistry.view<SpriteRendererComponent>());
    collect(srcRegistry.view<CameraComponent>());
    for (auto srcEntity : seen)
    {
        Entity srcEnt(srcEntity, &source);
        Entity dstEnt = target.CreateEntity();

        if (srcEnt.HasComponent<TagComponent>())
        {
            auto& srcTag = srcEnt.GetComponent<TagComponent>();
            dstEnt.AddComponent<TagComponent>().Tag = srcTag.Tag;
        }

        if (srcEnt.HasComponent<TransformComponent>())
        {
            auto& srcT = srcEnt.GetComponent<TransformComponent>();
            auto& dstT = dstEnt.AddComponent<TransformComponent>();
            dstT.SetPosition(srcT.Position);
            dstT.SetRotation(srcT.Rotation);
            dstT.SetScale(srcT.Scale);
        }

        if (srcEnt.HasComponent<SpriteRendererComponent>())
        {
            auto& srcS = srcEnt.GetComponent<SpriteRendererComponent>();
            auto& dstS = dstEnt.AddComponent<SpriteRendererComponent>();
            dstS.Color        = srcS.Color;
            dstS.Texture      = srcS.Texture;
            dstS.TexturePath  = srcS.TexturePath;
            dstS.TilingFactor = srcS.TilingFactor;
        }

        if (srcEnt.HasComponent<CameraComponent>())
        {
            auto& srcC = srcEnt.GetComponent<CameraComponent>();
            auto& dstC = dstEnt.AddComponent<CameraComponent>();
            dstC.Camera           = srcC.Camera;
            dstC.Primary          = srcC.Primary;
            dstC.FixedAspectRatio = srcC.FixedAspectRatio;
        }
    }
}

} // namespace Vulkitten
