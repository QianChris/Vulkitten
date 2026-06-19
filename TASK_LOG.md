# TASK_LOG.md — Vulkitten Engine Task Execution Log

## Task 1: Complete Basic RenderGraph Invocation
- **Start**: 2026-06-19
- **End**: 2026-06-19
- **Summary**: Implemented RenderGraph::Execute() with command filtering by pass name. Created PreparePass (ClearCommand → Legacy::RenderCommand::Clear), SpriteRenderPass (DrawQuadCommand → Renderer2D batch), EndPass (SwapBuffers via GraphicsContext). Wired passes in Renderer::Init(). Moved SwapBuffers from WindowsWindow::OnUpdate to EndPass. Updated Scene::OnUpdate to set camera on graph and skip direct RenderScene when systems are present. Updated RenderSystem to emit ClearCommand. Removed direct Clear calls from ExampleLayer2. Added RenderPass.cpp with SetExecute/Read/Write implementations. Added GetGraphicsContext() to Window interface. Fixed EditorLayer to use Legacy::RenderCommand. All 3 targets compile.

## Task 2: Framebuffer Configuration in Passes
- **Start**: 2026-06-19
- **End**: 2026-06-19
- **Summary**: Added SetFramebuffer/GetFramebuffer to RenderGraph. Updated PreparePass and SpriteRenderPass to bind/unbind framebuffer when configured (nullptr = default backbuffer). EditorLayer now sets viewport framebuffer on RenderGraph instead of manually binding it for the entire render. PreparePass clears the configured framebuffer, SpriteRenderPass draws to it. Entity ID attachment clear remains editor-specific. All 3 targets compile.

## Task 3: Clean Up Legacy Renderer2D Path
- **Start**: 2026-06-19
- **End**: 2026-06-19
- **Summary**: Removed Scene::RenderScene() method entirely. Removed legacy fallback path in Scene::OnUpdate() — all rendering now goes through RenderGraph. Created GpuParticlePass that iterates GPU emitter entities via Scene and calls Update+Render between PreparePass and SpriteRenderPass. Added SetScene/GetScene to RenderGraph. Added GetEmitterManager() public accessor to Scene. Removed Renderer2D.h include from Scene.h. All 3 targets compile.

## Task 4: ClassFactory Singleton
- **Start**: 2026-06-19
- **End**: 2026-06-19
- **Summary**: Created ClassFactory as Meyer's singleton — the single centralized access point for all engine subsystems. Provides GenerateUUID() (central RNG), GetApplication(), GetWindow(), GetRenderGraph(). Added to Vulkitten.h umbrella header. Preserves existing UUID class for standalone use. All 3 targets compile.
