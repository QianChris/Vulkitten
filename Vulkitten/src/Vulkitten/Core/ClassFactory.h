#pragma once

#include "Vulkitten/Core/Core.h"
#include "Vulkitten/Core/UUID.h"

#include <functional>
#include <typeindex>
#include <unordered_map>

namespace Vulkitten {

class Application;
class Window;
class RenderGraph;

// ============================================================
// ClassFactory - the single centralized singleton for the engine.
// All subsystem access and object creation routes through here.
// ============================================================
class VKT_API ClassFactory
{
public:
    static ClassFactory& Get();

    // ---- UUID Generation ----
    UUID GenerateUUID();

    // ============================================================
    // DI: Instance Management
    // ============================================================

    // Create a new instance of T via Meyer's singleton pattern.
    // The instance lives for the lifetime of the program.
    template<typename T>
    static T& CreateInstance()
    {
        static T instance;
        return instance;
    }

    // Register an externally-created instance pointer.
    // Registered instances take precedence over Meyer's singleton in GetInstance().
    template<typename T>
    static void RegisterInstance(T* instance)
    {
        Get().m_Instances[std::type_index(typeid(T))] = static_cast<void*>(instance);
    }

    // Get or create an instance of T.
    // Returns a registered instance if one exists; otherwise creates one
    // via CreateInstance<T>() (Meyer's singleton) and caches it.
    template<typename T>
    static T& GetInstance()
    {
        auto& instances = Get().m_Instances;
        auto it = instances.find(std::type_index(typeid(T)));
        if (it != instances.end())
            return *static_cast<T*>(it->second);

        // Lazy-create via Meyer's singleton on first access
        T& instance = CreateInstance<T>();
        instances[std::type_index(typeid(T))] = &instance;
        return instance;
    }

    // ============================================================
    // DI: Interface Management
    // ============================================================

    // Register an implementation type for an interface.
    // GetInterface<I>() will create Impl via GetInstance<Impl>() and return it as I&.
    template<typename I, typename Impl>
    static void RegisterInterface()
    {
        Get().m_InterfaceFactories[std::type_index(typeid(I))] = []() -> void* {
            return static_cast<I*>(&GetInstance<Impl>());
        };
    }

    // Register an existing instance as the implementation for an interface.
    template<typename I>
    static void RegisterInterface(I* impl)
    {
        Get().m_InterfaceFactories[std::type_index(typeid(I))] = [impl]() -> void* {
            return const_cast<void*>(static_cast<const void*>(impl));
        };
    }

    // Get the implementation registered for interface I.
    // Triggers a VKT_CORE_ASSERT if no implementation has been registered.
    template<typename I>
    static I& GetInterface()
    {
        auto& factories = Get().m_InterfaceFactories;
        auto it = factories.find(std::type_index(typeid(I)));
        VKT_CORE_ASSERT(it != factories.end(),
            "ClassFactory::GetInterface: no implementation registered for this interface!");
        return *static_cast<I*>(it->second());
    }

    // ---- Core Subsystem Access (existing, to be migrated to DI in later tasks) ----
    Application& GetApplication();
    Window& GetWindow();

    // ---- Renderer Subsystem Access ----
    RenderGraph* GetRenderGraph();

    // Prevent copies
    ClassFactory(const ClassFactory&) = delete;
    ClassFactory& operator=(const ClassFactory&) = delete;

private:
    ClassFactory() = default;
    ~ClassFactory() = default;

    // Type-erased instance storage: type_index → void*
    std::unordered_map<std::type_index, void*> m_Instances;
    // Type-erased interface factory storage: type_index → factory function
    std::unordered_map<std::type_index, std::function<void*()>> m_InterfaceFactories;
};

} // namespace Vulkitten
