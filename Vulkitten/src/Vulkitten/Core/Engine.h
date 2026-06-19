#pragma once

#include "Vulkitten/Core/Core.h"
#include "Vulkitten/Core/FileSystem.h"

namespace Vulkitten {

class Log;
class Input;

// ============================================================
// Engine — the core engine singleton.
//
// Created via ClassFactory::GetInstance<Engine>(). Owns all
// engine subsystems: FileSystem, Input lifecycle, Log reference,
// and future EventQueue / ThreadPool.
//
// Usage:
//   Engine::Get().Init();
//   auto& fs = Engine::Get().GetFileSystem();
//   Engine::Get().Shutdown();
// ============================================================
class VKT_API Engine
{
public:
    Engine();
    ~Engine();

    // Initialize engine subsystems (FileSystem paths, Input setup, etc.)
    void Init();

    // Shutdown and cleanup (Input teardown, flush queues, etc.)
    void Shutdown();

    // ---- Subsystem Accessors ----

    FileSystem& GetFileSystem() { return *m_FileSystem; }

    // Returns a reference to the Log singleton.
    // VKT_CORE_* / VKT_* macros remain the primary logging interface.
    static Log& GetLogger();

    // Convenience accessor — delegates to ClassFactory::GetInstance<Engine>()
    static Engine& Get();

    // ---- Future Subsystems (stubs) ----

    // Placeholder for a deferred/timestamped event queue.
    struct EventQueue {};

    // Placeholder for a worker thread pool.
    struct ThreadPool {};

    // Prevent copies
    Engine(const Engine&) = delete;
    Engine& operator=(const Engine&) = delete;

private:
    Scope<FileSystem> m_FileSystem;
    EventQueue m_EventQueue;
    ThreadPool m_ThreadPool;
};

} // namespace Vulkitten
