# AGENTS.md - Vulkitten Engine Development Guide

Guidelines for AI agents contributing to the Vulkitten Engine, a C++17 game engine.

---

## Project Overview
- **Standard**: C++17 | **Build**: CMake 3.16+ with Visual Studio 2022 (x64, Windows)
- **Architecture**: DLL (`Vulkitten`) + Executable (`SandBox`) | **Renderer**: OpenGL 4 (GLAD)
- **Key Dependencies**: spdlog, glm, glfw, imgui, stb_image

---

## Build, Lint, & Test Commands
### Build
```bash
# Quick debug build (x64)
build.bat
# Manual:
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Debug

# Build specific target
cmake --build build --target Vulkitten --config Debug
cmake --build build --target SandBox --config Release

# Clean and rebuild
cmake --build build --target clean --config Debug && cmake --build build --config Debug
```
Binaries: `bin/[Config]-x64/[ProjectName]/` (e.g., `bin/Debug-x64/SandBox/SandBox.exe`)

### Lint
Use vendored `.clang-format` (4 spaces, 100-char limit, CRLF endings).

### Test
No test framework configured. Verify changes via successful builds and manual execution of `SandBox.exe`.

---

## Code Style Guidelines
### Formatting & Structure
- **Braces**: K&R style, opening brace on new line
- **Access Modifiers**: No extra indent; members indented once

### Include Order
1. Corresponding header (for `.cpp`) | 2. Precompiled header (`vktpch.h`)
3. System headers (`<vector>`, `<string>`) | 4. Third-party headers (`<spdlog/spdlog.h>`, etc.)
5. Project headers (`"Vulkitten/Core.h"`)

### Naming Conventions
| Element | Convention | Example |
|---------|------------|---------|
| Namespace/Class/Struct | PascalCase | `Vulkitten` |
| Function | PascalCase | `BeginSession` |
| Variable | camelCase | `deltaTime` |
| Instance Member | `m_` + camelCase | `m_Width` |
| Static Member | `s_` + camelCase | `s_GLFWInitialized` |
| Constant | kPascalCase / UPPER_SNAKE | `kMaxEntities` |
| Enum/Enum Value | PascalCase | `EventType::KeyPressed` |
| Macro | UPPER_SNAKE_CASE | `VKT_API` |

### Error Handling
- **Assertions**: `VKT_ASSERT(cond, "msg")` (client) or `VKT_CORE_ASSERT` (engine)
- **Logging**: Use `VKT_CORE_*` (engine) / `VKT_*` (client) macros; use `.string()` for paths
- **DLL Exports**: Mark with `VKT_API`: `class VKT_API Application { ... };`

---

## Key Subsystems
- **Event System**: Inherit from `Event`, use `EVENT_CLASS_TYPE`/`EVENT_CLASS_CATEGORY` macros
- **Layers**: `PushLayer` (before overlays) or `PushOverlay` (always top)
- **Renderer**: Abstract `RendererAPI`; use `RenderCommand`, `Shader`, `Texture`, `VertexArray`
- **Entry Point**: Implement `Vulkitten::CreateApplication()` returning `Application*`

---

## Editor Architecture (VulkittenEditor)
### Panels
All panels in `VulkittenEditor/src/Panel/` follow:
```cpp
class PanelName {
public:
    PanelName() = default;
    PanelName(const Ref<Scene>& scene);
    void SetContext(const Ref<Scene>& scene);
    void OnImGuiRender();
};
```

| Panel | Purpose |
|-------|---------|
| `SceneHierarchyPanel` | Entity tree, selection (`m_SelectedEntityID`) |
| `PropertyPanel` | Edit selected entity's components |
| `ViewportPanel` | Render scene to framebuffer, gizmo |
| `PerformancePanel` | FPS, frame time, Renderer2D stats |
| `ResourcePanel` | Visualize texture resources |

### Panel Integration
Initialize in `DefaultLayer::OnAttach()`:
```cpp
m_SceneHierarchyPanel.SetContext(m_Scene);
m_PropertyPanel.SetContext(m_Scene);
```
Render order: ViewportPanel → SceneHierarchyPanel → PropertyPanel → ResourcePanel → PerformancePanel

---

## Texture & Serialization
- **SpriteRendererComponent**: Stores `Texture` (Ref) and `TexturePath` (string) for serialization
- **YAML**: `TexturePath: "sandbox://assets/textures/Checkerboard.png"`
- **Deserialization**: `sprite.Texture = Texture2D::Create(sprite.TexturePath);`
- **FileSystem**: `FileSystem::RegisterPath("../../Sandbox", "sandbox");`

---

## Template & Memory
- Template definitions must stay in headers
- Use `Ref<T>` for renderer resources
- Use RAII (`std::lock_guard`); avoid manual memory management

---

## Notes
- Only Windows (x64) + OpenGL supported
- Third-party libs vendored in `Vulkitten/vendor/`
- All `.cpp` files must include `vktpch.h` first
- Use `VKT_PROFILE_*` macros for profiling
- Commit format: `type: short description` (e.g., `feat: add 2D renderer`)