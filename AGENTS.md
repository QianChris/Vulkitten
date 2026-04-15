# AGENTS.md - Vulkitten Engine Development Guide

This file provides guidelines for AI agents working on the Vulkitten Engine codebase.

---

## Project Overview

**VulkittenEngine** is a C++17 game engine using:
- **C++ Standard**: C++17
- **Build System**: CMake 3.16+ with Visual Studio 2022
- **Platform**: Windows (x64)
- **Architecture**: Shared DLL (Vulkitten) + Executable (SandBox)

### Directory Structure

```
VulkittenEngine/
├── CMakeLists.txt          # Root CMake configuration
├── build.bat               # Quick build script (VS2022 x64)
├── bin/                    # Build outputs (Debug-x64/, Release-x64/)
├── bin-int/                # Intermediate files (.pdb, .obj)
├── Vulkitten/              # Engine core library
│   ├── src/Vulkitten/      # Core headers and sources
│   │   ├── Core.h          # Platform defines, API macros
│   │   ├── Log.h           # Logging system
│   │   ├── Assert.h        # Assert macros
│   │   ├── Application.h   # Application base class
│   │   ├── Window.h        # Window abstract class
│   │   ├── EntryPoint.h    # Main entry point
│   │   ├── vktpch.h        # Precompiled header
│   │   ├── Events/         # Event system (Event.h, *Event.h)
│   │   └── Platform/       # Platform implementations
│   │       └── Windows/    # Windows-specific code
│   └── vendor/             # Third-party (spdlog, glm, glfw)
└── SandBox/                # Example/editor application
    └── src/                # Application sources
```

---

## Build Commands

### Quick Build (Windows)

```bash
# Run the build script
build.bat

# Or manually:
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Debug
```

### Build Configurations

```bash
cmake --build build --config Debug        # Debug (default)
cmake --build build --config Release      # Release
cmake --build build --config RelWithDebInfo  # Optimized with debug info
```

### Build Specific Targets

```bash
cmake --build build --target Vulkitten --config Debug
cmake --build build --target SandBox --config Release
```

### Clean and Rebuild

```bash
cmake --build build --target clean --config Debug
cmake --build build --config Debug
```

### Output Locations

- **Binaries**: `bin/[Config]-x64/[ProjectName]/`
  - Example: `bin/Debug-x64/SandBox/SandBox.exe`
  - DLL: `bin/Debug-x64/Vulkitten/Vulkitten.dll`
- **Intermediate**: `bin-int/[Config]-x64/[ProjectName]/`

---

## Code Style Guidelines

### Formatting

- **Indentation**: 4 spaces (no tabs)
- **Column Limit**: 100 characters
- **Line Endings**: Windows-style (CRLF) or auto-detected
- A `.clang-format` style is vendored in `Vulkitten/vendor/spdlog/.clang-format`

### File Organization

```cpp
#pragma once

#include <system_headers>
#include "Vulkitten/headers"

namespace Vulkitten
{
    // declarations
}
```

### Naming Conventions

| Element | Convention | Example |
|---------|------------|---------|
| Namespace | PascalCase | `Vulkitten` |
| Class/Struct | PascalCase | `Application`, `Vector3` |
| Function | PascalCase | `CreateApplication`, `Initialize` |
| Variable | camelCase | `deltaTime`, `entityCount` |
| Static Member | camelCase with s_ prefix | `s_GLFWInitialized` |
| Instance Member | camelCase with m_ prefix | `m_Width`, `m_Window` |
| Constant | kPascalCase or UPPER_SNAKE | `kMaxEntities`, `MAX_BUFFER_SIZE` |
| Enum | PascalCase | `EventType` |
| Enum Value | PascalCase | `EventType::KeyPressed` |
| Plain Enum Value | PascalCase | `EventCategoryApplication` |
| Macro | UPPER_SNAKE_CASE | `BIT(x)`, `VULKITTEN_PLATFORM_WINDOWS` |

### Braces and Spacing

```cpp
// Function definition
void Initialize()
{
    if (condition)
    {
        DoSomething();
    }
    else
    {
        DoOtherThing();
    }
}

// Constructor initializer list
MyClass::MyClass(int value)
    : member_(value)
    , other_(0)
{
}
```

### Access Modifiers

- Order: `public:`, `protected:`, `private:` (no extra indent)
- Indent members same as access modifier

```cpp
class Example
{
public:
    Example();
    virtual ~Example();

protected:
    void ProtectedMethod();

private:
    int member_;
};
```

### Event System Macros

Use the provided macros for event class type/category registration:

```cpp
class WindowResizeEvent : public Event
{
public:
    EVENT_CLASS_TYPE(WindowResize)
    EVENT_CLASS_CATEGORY(EventCategoryApplication)
};
```

---

## Include Order

1. Corresponding header (for .cpp files)
2. Precompiled header (`vktpch.h`)
3. System headers (`<stdio.h>`, `<vector>`, etc.)
4. Third-party library headers (`<spdlog/spdlog.h>`, `<GLFW/glfw3.h>`)
5. Project headers (`"Vulkitten/Core.h"`, `"<Vulkitten.h>"`)

---

## DLL Export/Import

Use the `VKT_API` macro for cross-DLL symbols (defined in `Core.h`):

```cpp
#ifdef VULKITTEN_BUILD_DLL
    #define VKT_API __declspec(dllexport)
#else
    #define VKT_API __declspec(dllimport)
#endif

class VKT_API Application
{
};
```

---

## Logging

The engine uses spdlog via `Vulkitten/Log.h`.

### Log Macros

**Core logger** (engine internal):
```cpp
VKT_CORE_ERROR("Error: {}", details);
VKT_CORE_WARN("Warning: {}", details);
VKT_CORE_INFO("Info: {}", details);
VKT_CORE_DEBUG("Debug: {}", details);
VKT_CORE_TRACE("Trace: {}", details);
```

**Client logger** (application):
```cpp
VKT_ERROR("Error: {}", details);
VKT_WARN("Warning: {}", details);
VKT_INFO("Info: {}", details);
VKT_DEBUG("Debug: {}", details);
VKT_TRACE("Trace: {}", details);
```

---

## Assertions

Assertions are controlled by `VULKITTEN_ENABLE_ASSERTS` (default: enabled in CMakeLists.txt):

```cpp
#include "Vulkitten/Assert.h"

VKT_ASSERT(condition, "Error message: {0}", value);
VKT_CORE_ASSERT(condition, "Error message: {0}", value);
```

---

## Window System

### Abstract Window (Vulkitten/Window.h)

```cpp
class VKT_API Window
{
public:
    using EventCallbackFn = std::function<void(Event&)>;
    
    virtual void OnUpdate() = 0;
    virtual unsigned int GetWidth() const = 0;
    virtual unsigned int GetHeight() const = 0;
    virtual void SetEventCallback(const EventCallbackFn& callback) = 0;
    virtual void SetVSync(bool enabled) = 0;
    virtual bool IsVSync() const = 0;
    
    static Window* Create(const WindowProps& props = WindowProps());
};
```

### Window Props

```cpp
struct WindowProps
{
    std::string Title;
    unsigned int Width;
    unsigned int Height;
    
    WindowProps(const std::string& title = "Vulkitten Engine",
                unsigned int width = 1280,
                unsigned int height = 720);
};
```

---

## Event System

### Application Event Callbacks

Override event handlers in your Application subclass:

```cpp
class Sandbox : public Vulkitten::Application
{
public:
    Sandbox()
    {
        m_Window->SetEventCallback(BIND_EVENT_FN(Sandbox::OnEvent));
    }
    
    void OnEvent(Vulkitten::Event& e)
    {
        // Handle events
    }
    
private:
    bool OnWindowClose(Vulkitten::WindowCloseEvent& e)
    {
        m_Running = false;
        return true; // Event handled
    }
};
```

### Event Binding Macro

Use `BIND_EVENT_FN` to bind member functions:

```cpp
#define BIND_EVENT_FN(fn) [this](auto&&... args) -> decltype(auto) { return this->fn(std::forward<decltype(args)>(args)...); }
```

### Supported Events

- **Window**: `WindowCloseEvent`, `WindowResizeEvent`, `WindowFocusEvent`, `WindowMovedEvent`
- **Keyboard**: `KeyPressedEvent`, `KeyReleasedEvent`, `KeyTypedEvent`
- **Mouse**: `MouseButtonPressedEvent`, `MouseButtonReleasedEvent`, `MouseMovedEvent`, `MouseScrolledEvent`

---

## Application Entry Point

Applications implement `CreateApplication()` in the `Vulkitten` namespace:

```cpp
#include <Vulkitten.h>

class Sandbox : public Vulkitten::Application
{
public:
    Sandbox()
    {
        m_Window->SetEventCallback(BIND_EVENT_FN(Sandbox::OnEvent));
    }
    
    void OnEvent(Vulkitten::Event& e)
    {
        if (e.GetEventType() == Vulkitten::EventType::WindowClose)
            m_Running = false;
    }
};

Vulkitten::Application* Vulkitten::CreateApplication()
{
    return new Sandbox();
}
```

---

## Third-Party Libraries

| Library | Location | Purpose |
|---------|----------|---------|
| spdlog | `vendor/spdlog/` | Logging |
| glm | `vendor/glm/` | Math (vectors, matrices) |
| glfw | `vendor/glfw/` | Window and input |

---

## Commit Guidelines

- Format: `type: short description`
  - `feat: add render system`
  - `fix: resolve memory leak in texture loader`
  - `refactor: simplify vertex buffer interface`
- Keep commits focused and atomic
- Test changes before committing

---

## Notes

- No test framework configured yet
- spdlog, glm, and glfw are vendored in `Vulkitten/vendor/`
- The engine entry point is defined in `EntryPoint.h`
- Only Windows platform is currently supported
