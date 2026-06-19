#include "vktpch.h"
#include "Engine.h"

#include "Vulkitten/Core/ClassFactory.h"
#include "Vulkitten/Core/Input.h"
#include "Vulkitten/Core/Log.h"
#include "Vulkitten/Perf/Instrumentor.h"

namespace Vulkitten {

Engine::Engine()
    : m_FileSystem(CreateScope<FileSystem>())
{
}

Engine::~Engine() = default;

void Engine::Init()
{
    VKT_PROFILE_FUNCTION();

    VKT_CORE_INFO("Engine::Init — engine subsystems initializing...");

    // FileSystem: virtual paths are currently registered in Application::Application().
    // In a future task, RegisterPath calls will move here.
    // For now, the FileSystem instance is ready but empty — paths are added externally.

    // Input: currently initialized as a global static (WindowsInput.cpp line 10).
    // Engine owns the lifecycle: Shutdown() will clean it up.
    // Future: Engine::Init() will create the platform Input implementation and
    // set Input::s_Instance, removing the global static initializer.

    VKT_CORE_INFO("Engine::Init — complete.");
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

} // namespace Vulkitten
