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
