# AGENTS.md - Vulkitten Engine Development Guide

Guidelines for AI agents contributing to the Vulkitten Engine, a C++17 game engine.

---

## Project Overview
- **Standard**: C++17
- **Build**: CMake 3.16+ with Visual Studio 2022 (x64, Windows only)
- **Architecture**: Shared DLL (`Vulkitten`) + Executable (`SandBox`)
- **Renderer**: OpenGL 4 (via GLAD)
- **Key Dependencies**: spdlog (log), glm (math), glfw (window), imgui (GUI), stb_image (images)

---

## Build, Lint, & Test Commands
### Build
```bash
# Quick debug build (x64)
build.bat
# Manual equivalent:
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Debug

# Build specific target
cmake --build build --target Vulkitten --config Debug
cmake --build build --target SandBox --config Release

# Clean and rebuild
cmake --build build --target clean --config Debug
cmake --build build --config Debug
```
Binaries: `bin/[Config]-x64/[ProjectName]/` (e.g., `bin/Debug-x64/SandBox/SandBox.exe`)

### Lint
Use vendored `.clang-format` (from `vendor/spdlog/`):
- 4 spaces indentation, no tabs
- 100-character column limit
- Windows-style (CRLF) line endings

### Test
No test framework configured yet. Verify changes via successful builds and manual execution of `SandBox.exe`.
*Single test run (once configured):* Document standard method here when added.

---

## Code Style Guidelines
### Formatting & Structure
- **Braces**: K&R style, opening brace on new line:
  ```cpp
  void Initialize()
  {
      if (condition)
      {
          DoSomething();
      }
  }
  ```
- **Access Modifiers**: `public:`, `protected:`, `private:` with no extra indent; members indented once.

### Include Order
1. Corresponding header (for `.cpp` files)
2. Precompiled header (`vktpch.h`)
3. System headers (`<vector>`, `<string>`)
4. Third-party headers (`<spdlog/spdlog.h>`, `<GLFW/glfw3.h>`)
5. Project headers (`"Vulkitten/Core.h"`)

### Naming Conventions
| Element | Convention | Example |
|---------|------------|---------|
| Namespace/Class/Struct | PascalCase | `Vulkitten`, `OrthographicCamera` |
| Function | PascalCase | `BeginSession`, `CreateApplication` |
| Variable | camelCase | `deltaTime`, `entityCount` |
| Instance Member | `m_` prefix + camelCase | `m_Width`, `m_CurrentSession` |
| Static Member | `s_` prefix + camelCase | `s_GLFWInitialized` |
| Constant | kPascalCase / UPPER_SNAKE | `kMaxEntities`, `MAX_BUFFER_SIZE` |
| Enum/Enum Value | PascalCase | `EventType`, `EventType::KeyPressed` |
| Macro | UPPER_SNAKE_CASE | `VKT_API`, `BIT(x)`, `VKT_PROFILE_FUNCTION()` |

### Error Handling
- **Assertions**: Use `VKT_ASSERT(cond, "msg")` (client) or `VKT_CORE_ASSERT` (engine), controlled by `VULKITTEN_ENABLE_ASSERTS`.
- **Logging**: Use `VKT_CORE_*` (engine) / `VKT_*` (client) macros (spdlog backend). Avoid raw `c_str()` in logs; use `.string()` for `std::filesystem::path`.
- **DLL Exports**: Mark cross-DLL symbols with `VKT_API` (defined in `Core.h`):
  ```cpp
  class VKT_API Application { ... };
  ```

---

## Key Subsystems
- **Event System**: Inherit from `Event`, use `EVENT_CLASS_TYPE`/`EVENT_CLASS_CATEGORY` macros. Bind handlers with `BIND_EVENT_FN`.
- **Layers**: Push to `LayerStack` via `PushLayer` (before overlays) or `PushOverlay` (always top).
- **Renderer**: Abstract `RendererAPI` (only OpenGL 4). Use `RenderCommand` for draw calls, `Shader`/`Texture`/`VertexArray` for resources.
- **Entry Point**: Implement `Vulkitten::CreateApplication()` returning `Application*` subclass.

---

## Editor Architecture (VulkittenEditor)
### Panel System
Panels are standalone UI components in `VulkittenEditor/src/Panel/`. All panels follow a consistent interface:

```cpp
class PanelName {
public:
    PanelName() = default;
    PanelName(const Ref<Scene>& scene);           // Optional constructor with context
    void SetContext(const Ref<Scene>& scene);     // Set/update scene context
    void OnImGuiRender();                          // Per-frame UI rendering
};
```

**Available Panels:**
| Panel | Header File | Purpose |
|-------|------------|---------|
| `SceneHierarchyPanel` | `Panel/SceneHierarchyPanel.h` | Entity tree, handles selection, stores `m_SelectedEntityID` |
| `PropertyPanel` | `Panel/PropertyPanel.h` | Edit selected entity's components |
| `ViewportPanel` | `Panel/ViewportPanel.h` | Render scene to framebuffer, gizmo interaction |
| `PerformancePanel` | `Panel/PerformancePanel.h` | FPS, frame time, Renderer2D stats |
| `ResourcePanel` | `Panel/ResourcePanel.h` | Visualize texture resources in scene |

### ResourcePanel
The `ResourcePanel` displays all textures used in the current scene:
- Collects textures from `SpriteRendererComponent` via scene registry view
- Shows texture path, dimensions (WxH), and usage count
- Renders a 64x64 preview image for each texture
- Updates every frame by re-collecting resources

```cpp
struct TextureResource {
    std::string Path;
    Ref<Texture2D> Texture;
    uint32_t Width, Height;
    uint32_t UsageCount = 0;
};
```

### Panel Integration
Panels are members of `DefaultLayer` and initialized in `OnAttach()`:
```cpp
m_SceneHierarchyPanel.SetContext(m_Scene);
m_PropertyPanel.SetContext(m_Scene);
m_ViewportPanel.SetContext(m_Scene);
m_ResourcePanel.SetContext(m_Scene);
```

Render order in `OnImguiRender()`:
1. ViewportPanel (framebuffer rendering)
2. SceneHierarchyPanel (entity selection)
3. PropertyPanel (component editing, reads from SceneHierarchyPanel)
4. ResourcePanel (texture visualization)
5. PerformancePanel (stats overlay)

### Entity Selection Flow
`SceneHierarchyPanel` is the source of truth for entity selection. Other panels query via `GetSelectedEntity()`:
```cpp
m_PropertyPanel.SetSelectedEntity(m_SceneHierarchyPanel.GetSelectedEntity());
m_ViewportPanel.SetSelectedEntity(m_SceneHierarchyPanel.GetSelectedEntity());
```

---

## Texture & Serialization
### Texture2D Interface
`Texture2D` has a `GetPath()` method returning the file path (useful for serialization):
```cpp
virtual const std::string& GetPath() const override { return m_Path; }
```

### SpriteRendererComponent
Stores both the texture reference and its path for serialization:
```cpp
struct SpriteRendererComponent {
    glm::vec4 Color{ 1.0f, 1.0f, 1.0f, 1.0f };
    Ref<Texture2D> Texture{ nullptr };
    std::string TexturePath;  // Serialization key
    float TilingFactor{ 1.0f };
};
```

### Scene Serialization (YAML)
SpriteRendererComponent serializes texture path:
```yaml
SpriteRendererComponent:
  Color: [1.0, 1.0, 1.0, 1.0]
  TexturePath: "sandbox://assets/textures/Checkerboard.png"
  TilingFactor: 10.0
```

Deserialization reloads texture from path:
```cpp
sprite.TexturePath = spriteNode["TexturePath"].as<std::string>();
if (!sprite.TexturePath.empty()) {
    sprite.Texture = Texture2D::Create(sprite.TexturePath);
}
```

### FileSystem Protocol
Use `sandbox://` protocol for asset paths. Register paths in application entry:
```cpp
FileSystem::RegisterPath("../../Sandbox", "sandbox");
FileSystem::Resolve("sandbox://assets/textures/foo.png");  // Returns full path
```

---

## Template & Memory Guidelines
- Template definitions (e.g., `InstrumentorUtils::CleanupOutputString`) must remain in headers.
- Move non-template implementations to `.cpp` files to reduce header bloat.
- Use `Ref` template (reference counting) for renderer resources: `Ref<Shader>`, `Ref<VertexArray>`.
- Use RAII (e.g., `std::lock_guard` for mutexes); avoid manual memory management where possible.

---

## Commit Guidelines
- Format: `type: short description` (e.g., `feat: add 2D renderer`, `fix: resolve texture leak`)
- Keep commits atomic and focused; verify builds pass before committing.

---

## Notes
- Only Windows (x64) and OpenGL (via GLAD) are currently supported.
- Third-party libraries are vendored in `Vulkitten/vendor/`.
- Profile instrumentation enabled by default; use `VKT_PROFILE_*` macros to instrument scopes.
- All `.cpp` files must include `vktpch.h` first (precompiled header).
