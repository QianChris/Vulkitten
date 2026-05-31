# 目录结构: `src`

**完整路径**: `D:\Projects\VulkittenEngine\Vulkitten\src`
**生成时间**: The system cannot accept the date entered.
Enter the new date: (mm-dd-yy)

```
src
├── Platform
│   ├── OpenGL
│   │   ├── OpenGLBuffer.cpp
│   │   ├── OpenGLBuffer.h
│   │   ├── OpenGLContext.cpp
│   │   ├── OpenGLContext.h
│   │   ├── OpenGLFramebuffer.cpp
│   │   ├── OpenGLFramebuffer.h
│   │   ├── OpenGLRendererAPI.cpp
│   │   ├── OpenGLRendererAPI.h
│   │   ├── OpenGLShader.cpp
│   │   ├── OpenGLShader.h
│   │   ├── OpenGLTexture.cpp
│   │   ├── OpenGLTexture.h
│   │   ├── OpenGLUtil.cpp
│   │   ├── OpenGLUtil.h
│   │   ├── OpenGLVertexArray.cpp
│   │   └── OpenGLVertexArray.h
│   └── Windows
│       ├── WindowsFileDialogs.cpp
│       ├── WindowsInput.cpp
│       ├── WindowsInput.h
│       ├── WindowsWindow.cpp
│       └── WindowsWindow.h
├── Vulkitten
│   ├── Core
│   │   ├── Application.cpp
│   │   ├── Application.h
│   │   ├── Assert.h
│   │   ├── Core.h
│   │   ├── EntryPoint.h
│   │   ├── FileSystem.cpp
│   │   ├── FileSystem.h
│   │   ├── Input.h
│   │   ├── KeyCode.h
│   │   ├── Layer.cpp
│   │   ├── Layer.h
│   │   ├── LayerStack.cpp
│   │   ├── LayerStack.h
│   │   ├── Log.cpp
│   │   ├── Log.h
│   │   ├── MouseButtonCode.h
│   │   ├── Timestep.h
│   │   └── Window.h
│   ├── Events
│   │   ├── ApplicationEvent.h
│   │   ├── Event.h
│   │   ├── KeyEvent.h
│   │   └── MouseEvent.h
│   ├── ImGui
│   │   ├── ImGuiBuild.cpp
│   │   ├── ImGuiLayer.cpp
│   │   └── ImGuiLayer.h
│   ├── Perf
│   │   ├── Instrumentor.cpp
│   │   ├── Instrumentor.h
│   │   ├── Timer.cpp
│   │   └── Timer.h
│   ├── Renderer
│   │   ├── Buffer.cpp
│   │   ├── Buffer.h
│   │   ├── Camera.h
│   │   ├── CameraController.cpp
│   │   ├── CameraController.h
│   │   ├── EditorCamera.cpp
│   │   ├── EditorCamera.h
│   │   ├── Framebuffer.cpp
│   │   ├── Framebuffer.h
│   │   ├── GraphicsContext.h
│   │   ├── OrthographicCamera.cpp
│   │   ├── OrthographicCamera.h
│   │   ├── RenderCommand.cpp
│   │   ├── RenderCommand.h
│   │   ├── Renderer.cpp
│   │   ├── Renderer.h
│   │   ├── Renderer2D.cpp
│   │   ├── Renderer2D.h
│   │   ├── RendererAPI.cpp
│   │   ├── RendererAPI.h
│   │   ├── Shader.cpp
│   │   ├── Shader.h
│   │   ├── Texture.cpp
│   │   ├── Texture.h
│   │   ├── VertexArray.cpp
│   │   └── VertexArray.h
│   ├── Scene
│   │   ├── GpuParticle
│   │   │   ├── GpuParticle.cpp
│   │   │   ├── GpuParticle.h
│   │   │   ├── ParticleFunc.h
│   │   │   ├── ParticleInOut.h
│   │   │   └── ParticleStruct.h
│   │   ├── Components.h
│   │   ├── Entity.cpp
│   │   ├── Entity.h
│   │   ├── Scene.cpp
│   │   ├── Scene.h
│   │   ├── SceneCamera.cpp
│   │   ├── SceneCamera.h
│   │   ├── SceneSerializer.cpp
│   │   ├── SceneSerializer.h
│   │   └── ScriptableEntity.h
│   └── Utils
│       ├── FileDialogs.h
│       └── YAMLConversions.h
├── vktpch.cpp
├── vktpch.h
├── Vulkitten.h
└── VulkittenEntry.h
```