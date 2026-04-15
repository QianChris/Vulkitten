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
│   │   ├── Application.h   # Application base class
│   │   ├── EntryPoint.h    # Main entry point
│   │   └── Events/         # Event system (Event.h, *Event.h)
│   └── vendor/             # Third-party (spdlog, glm)
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
| Static Member | camelCase with s_ prefix | `s_CoreLogger` |
| Instance Member | camelCase with m_ prefix | `m_Width`, `m_Handled` |
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
2. System headers (`<stdio.h>`, `<vector>`, etc.)
3. Third-party library headers (`<spdlog/spdlog.h>`)
4. Project headers (`"Vulkitten/Core.h"`, `"<Vulkitten.h>"`)

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

### Usage

```cpp
#include <Vulkitten.h>  // Or: #include "Vulkitten/Log.h"

Application::Application()
{
    Vulkitten::Log::Initialize();
}
```

---

## Error Handling

- Use exceptions sparingly; prefer return codes for performance-critical code
- Log errors using the provided logging macros
- Platform check macro: `VULKITTEN_PLATFORM_WINDOWS`

---

## Header Guidelines

### Always Use Pragma Once

```cpp
#pragma once
```

### Minimize Includes

- Use forward declarations when possible
- Include only what's necessary for declaration

### Public vs Internal Headers

- Public API: `Vulkitten/src/Vulkitten/*.h`
- Internal headers: Same directory as source

---

## Application Entry Point

Applications implement `CreateApplication()` in the `Vulkitten` namespace:

```cpp
#include <Vulkitten.h>

class Sandbox : public Vulkitten::Application
{
public:
    Sandbox() {}
    ~Sandbox() {}
};

Vulkitten::Application* Vulkitten::CreateApplication()
{
    return new Sandbox();
}
```

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
- spdlog and glm are vendored in `Vulkitten/vendor/`
- The engine entry point is defined in `EntryPoint.h`
- Only Windows platform is currently supported
