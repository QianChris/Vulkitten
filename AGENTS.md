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
├── AGENTS.md              # Development guide (this file)
├── bin/                  # Build outputs (Debug-x64/, Release-x64/)
├── bin-int/               # Intermediate files (.pdb, .obj)
├── Vulkitten/            # Engine core library
│   ├── src/Vulkitten/
│   │   ├── Core.h           # Platform defines, API macros
│   │   ├── Log.h            # Logging system
│   │   ├── Assert.h         # Assert macros
│   │   ├── Application.h    # Application base class
│   │   ├── Window.h         # Window abstract class
│   │   ├── Input.h          # Input abstraction
│   │   ├── Layer.h          # Layer base class
│   │   ├── LayerStack.h     # Layer stack container
│   │   ├── EntryPoint.h    # Main entry point
│   │   ├── vktpch.h        # Precompiled header
│   │   ├── KeyCode.h       # Keyboard key codes
│   │   ├── MouseButtonCode.h # Mouse button codes
│   │   ├── Core/            # Core utilities
│   │   │   └── Timestep.h  # Time step for deltaTime
│   │   ├── Events/          # Event system
│   │   │   ├── Event.h         # Base event class
│   │   │   ├── ApplicationEvent.h
│   │   │   ├── KeyEvent.h
│   │   │   └── MouseEvent.h
│   │   ├── Renderer/        # Rendering subsystem
│   │   │   ├── Renderer.h       # Renderer singleton
│   │   │   ├── RendererAPI.h    # Abstract API
│   │   │   ├── RenderCommand.h  # Render commands
│   │   │   ├── Shader.h
│   │   │   ├── Texture.h
│   │   │   ├── Buffer.h       # Vertex/Index buffers
│   │   │   ├── VertexArray.h
│   │   │   ├── GraphicsContext.h
│   │   │   └── OrthographicCamera.h
│   │   ├── ImGui/           # ImGui integration
│   │   │   ├── ImGuiLayer.h
│   │   │   └── ImGuiBuild.cpp
│   │   └── Platform/        # Platform implementations
│   │       ├── OpenGL/      # OpenGL renderer
│   │       │   ├── OpenGLContext.h
│   │       │   ├── OpenGLShader.h
│   │       │   ├── OpenGLTexture.h
│   │       │   ├── OpenGLBuffer.h
│   │       │   ├── OpenGLVertexArray.h
│   │       │   ├── OpenGLRendererAPI.h
│   │       │   └── OpenGLUtil.h
│   │       └── Windows/     # Windows platform
│   │           ├── WindowsWindow.h
│   │           └── WindowsInput.h
│   └── vendor/            # Third-party libraries
│       ├── spdlog/        # Logging
│       ├── glm/           # Math library
│       ├── glfw/          # Window/input
│       ├── glad/          # OpenGL loader
│       ├── imgui/         # GUI library
│       └── stb_image/     # Image loading
└── Sandbox/              # Example application
    └── src/
        └── SandboxApp.cpp
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
- A `.clang-format` style is vendored in `vendor/spdlog/.clang-format`

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

### Notes

- Do NOT pass raw pointers to logging (e.g., `c_str()`, `filesystem::path::c_str()`)
- Use `.string()` or similar to convert to std::string before logging

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

## Input System

### Input Class (Vulkitten/Input.h)

```cpp
class Input
{
public:
    static bool IsKeyPressed(KeyCode key);
    static bool IsMouseButtonPressed(MouseCode button);
    static std::pair<float, float> GetMousePosition();
    static float GetMouseX();
    static float GetMouseY();
};
```

### Key Codes

Key codes are defined in `Vulkitten/KeyCode.h` (e.g., `KeyCode::Space`, `KeyCode::A`, etc.)

### Mouse Codes

Mouse button codes are in `Vulkitten/MouseButtonCode.h` (e.g., `MouseButton::Left`, `MouseButton::Right`, etc.)

---

## Layer System

### Layer Base Class

Layers provide a way to organize application functionality:

```cpp
class VKT_API Layer
{
public:
    Layer(const std::string& name = "Layer");
    virtual ~Layer();

    virtual void OnAttach() {}      // Called when layer is added
    virtual void OnDetach() {}    // Called when layer is removed
    virtual void OnUpdate() {}   // Called every frame
    virtual void OnEvent(Event& event) {}  // Handle events

    inline const std::string& GetName() const { return m_DebugName; }
};
```

### LayerStack

The LayerStack manages layer ordering. Layers are updated in order; overlays are always on top:

```cpp
class VKT_API Application
{
public:
    void PushLayer(Layer* layer);      // Insert before overlays
    void PushOverlay(Layer* overlay);  // Always on top
};
```

---

## Renderer System

### Renderer (Vulkitten/Renderer/Renderer.h)

The Renderer is a singleton that manages rendering:

```cpp
class Renderer
{
public:
    static void Init();
    static void Shutdown();
    static void BeginFrame();
    static void EndFrame();
    static void Submit(const Ref<VertexArray>& vertexArray);
    static void OnWindowResize(uint32_t width, uint32_t height);
    static RendererAPI GetAPI();
};
```

### RendererAPI

Abstract rendering API. Currently supported:
- `RendererAPI::OpenGL4`

### Render Command

```cpp
class RenderCommand
{
public:
    static void SetClearColor(const glm::vec4& color);
    static void Clear();
    static void DrawIndexed(const Ref<VertexArray>& va);
};
```

### Shader (Vulkitten/Renderer/Shader.h)

```cpp
class Shader : public RefCounted
{
public:
    static Ref<Shader> Create(const std::string& filepath);
    static Ref<Shader> Create(const std::string& name, const std::string& vertexSrc, const std::string& fragmentSrc);
    virtual void Bind() const = 0;
    virtual void UploadUniform(const std::string& name, int value) = 0;
    virtual void UploadUniform(const std::string& name, const glm::mat4& matrix) = 0;
};
```

### Texture (Vulkitten/Renderer/Texture.h)

```cpp
class Texture2D : public RefCounted
{
public:
    virtual ~Texture2D() = default;
    virtual void Bind(uint32_t slot = 0) const = 0;
    virtual uint32_t GetWidth() const = 0;
    virtual uint32_t GetHeight() const = 0;
};
```

### VertexArray (Vulkitten/Renderer/VertexArray.h)

```cpp
class VertexArray : public RefCounted
{
public:
    virtual ~VertexArray() = default;
    virtual void Bind() const = 0;
    virtual void Unbind() const = 0;
    virtual void AddVertexBuffer(const Ref<VertexBuffer>& vb) = 0;
    virtual void SetIndexBuffer(const Ref<IndexBuffer>& ib) = 0;
};
```

### Buffer (Vulkitten/Renderer/Buffer.h)

```cpp
class VertexBuffer : public RefCounted
{
public:
    static Ref<VertexBuffer> Create(float* vertices, uint32_t size);
    virtual void Bind() const = 0;
};

class IndexBuffer : public RefCounted
{
public:
    static Ref<IndexBuffer> Create(uint32_t* indices, uint32_t count);
    virtual void Bind() const = 0;
    virtual uint32_t GetCount() const = 0;
};
```

### Orthographic Camera

```cpp
class OrthographicCamera
{
public:
    OrthographicCamera(float left, float right, float bottom, float top);
    void SetProjection(float left, float right, float bottom, float top);
    const glm::mat4& GetProjectionMatrix() const;
    const glm::mat4& GetViewMatrix() const;
    void SetPosition(const glm::vec3& position);
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

## Timestep

Use `Timestep` for frame-time calculations:

```cpp
void Layer::OnUpdate(Timestep ts)
{
    float delta = ts.GetSeconds();
}
```

---

## Third-Party Libraries

| Library | Location | Purpose |
|---------|----------|---------|
| spdlog | `vendor/spdlog/` | Logging |
| glm | `vendor/glm/` | Math (vectors, matrices) |
| glfw | `vendor/glfw/` | Window and input |
| imgui | `vendor/imgui/` | GUI library |
| glad | `vendor/glad/` | OpenGL function loader |
| stb_image | `vendor/stb_image/` | Image loading |

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
- Libraries are vendored in `Vulkitten/vendor/`
- The engine entry point is defined in `EntryPoint.h`
- Only OpenGL (via GLAD) and Windows platforms are currently supported