// ============================================================
// VulkittenRHI Sample — Clear-to-Red + Triangle
//
// Switch between OpenGL and Vulkan by changing ONE line:
//   const rhi::BackendType backend = rhi::BackendType::OpenGL;
//   const rhi::BackendType backend = rhi::BackendType::Vulkan;
// ============================================================

#include <rhi/Renderer.hpp>
#include <rhi/IRenderDevice.hpp>
#include <rhi/ICommandBuffer.hpp>
#include <rhi/RenderCommandList.hpp>
#include <rhi/ISurface.hpp>
#include <rhi/Core/Types.hpp>
#include <rhi/Core/Format.hpp>
#include <rhi/ResourceDescs.hpp>

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <vector>
#include <fstream>

// ============================================================
// GLFWSurface
// ============================================================
class GLFWSurface : public rhi::ISurface
{
public:
    explicit GLFWSurface(GLFWwindow* window) : m_Window(window) {
        glfwGetFramebufferSize(window, &m_Width, &m_Height);
    }
    rhi::SurfaceDesc GetDesc() const override {
        return {static_cast<uint32_t>(m_Width), static_cast<uint32_t>(m_Height)};
    }
    void* GetNativeHandle() const override { return static_cast<void*>(m_Window); }
    void UpdateSize() { glfwGetFramebufferSize(m_Window, &m_Width, &m_Height); }
private:
    GLFWwindow* m_Window = nullptr;
    int m_Width = 800, m_Height = 600;
};

static void GlfwErrorCallback(int error, const char* description) {
    fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

// ============================================================
// Read file into vector
// ============================================================
static std::vector<uint8_t> ReadFile(const char* path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file) {
        fprintf(stderr, "Failed to open: %s\n", path);
        return {};
    }
    size_t size = static_cast<size_t>(file.tellg());
    file.seekg(0);
    std::vector<uint8_t> data(size);
    file.read(reinterpret_cast<char*>(data.data()), size);
    return data;
}

// ============================================================
// Vertex data
// ============================================================
struct Vertex {
    float pos[3];
    float color[3];
};

// Full-screen triangle — covers entire viewport, impossible to miss
static const Vertex kVertices[3] = {
    {{-0.5f, 0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}},  // bottom-left, red
    {{ 0.0f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}},  // bottom-right, green
    {{0.5f,  0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}},  // top-left, blue
};

struct UniformBufferObject {
    glm::mat4 projection;
    glm::mat4 model;
    glm::mat4 view;
};

// ============================================================
// main
// ============================================================
int main(int argc, char* argv[])
{
    // ---- Select Backend: --vulkan (default) or --opengl ----
    rhi::BackendType backend = rhi::BackendType::Vulkan;
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--opengl") == 0 || strcmp(argv[i], "-gl") == 0)
            backend = rhi::BackendType::OpenGL;
        else if (strcmp(argv[i], "--vulkan") == 0 || strcmp(argv[i], "-vk") == 0)
            backend = rhi::BackendType::Vulkan;
    }

    // ---- GLFW ----
    glfwSetErrorCallback(GlfwErrorCallback);
    if (!glfwInit()) { fprintf(stderr, "GLFW init failed\n"); return 1; }

    if (backend == rhi::BackendType::OpenGL) {
        glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    } else {
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    }

    GLFWwindow* window = glfwCreateWindow(800, 600, "VulkittenRHI — Triangle", nullptr, nullptr);
    if (!window) { glfwTerminate(); return 1; }

    GLFWSurface surface(window);

    // ---- Create Renderer ----
    rhi::RendererConfig config;
    config.Backend = backend;
    config.Surface = &surface;

    std::unique_ptr<rhi::Renderer> renderer;
    try { renderer = rhi::Renderer::Create(config); }
    catch (const std::exception& e) {
        fprintf(stderr, "Renderer failed: %s\n", e.what());
        glfwDestroyWindow(window); glfwTerminate(); return 1;
    }
    printf("Backend: %s\n", renderer->GetBackendType() == rhi::BackendType::OpenGL ? "OpenGL" : "Vulkan");

    // ---- Load shaders (SPIR-V works for both Vulkan and OpenGL 4.6 with GL_ARB_gl_spirv) ----
    std::vector<uint8_t> vsData, fsData;
    vsData = ReadFile("../Vulkitten/assets/shaders/Triangle.vert.spv");
    fsData = ReadFile("../Vulkitten/assets/shaders/Triangle.frag.spv");
    if (vsData.empty() || fsData.empty()) { fprintf(stderr, "Shader load failed\n"); return 1; }

    // ---- Create resources ----
    rhi::IRenderDevice& device = renderer->GetDevice();

    // Vertex buffer
    rhi::BufferDesc vbDesc;
    vbDesc.Size = sizeof(kVertices);
    vbDesc.Usage = rhi::BufferUsage::Vertex;
    vbDesc.Memory = rhi::MemoryProperty::HostVisible;
    rhi::BufferHandle vb = device.CreateBuffer(vbDesc, kVertices);

    // Uniform buffer
    UniformBufferObject ubo{};
    ubo.projection = glm::mat4(1.0f);
    ubo.model = glm::mat4(1.0f);
    ubo.view = glm::mat4(1.0f);

    rhi::BufferDesc ubDesc;
    ubDesc.Size = sizeof(UniformBufferObject);
    ubDesc.Usage = rhi::BufferUsage::Uniform;
    ubDesc.Memory = rhi::MemoryProperty::HostVisible;
    rhi::BufferHandle ub = device.CreateBuffer(ubDesc, &ubo);

    // Shaders
    rhi::ShaderBytecode vsBc{vsData.data(), vsData.size(), "main"};
    rhi::ShaderBytecode fsBc{fsData.data(), fsData.size(), "main"};
    rhi::ShaderHandle vs = device.CreateShader(rhi::ShaderStage::Vertex, vsBc);
    rhi::ShaderHandle fs = device.CreateShader(rhi::ShaderStage::Fragment, fsBc);
    if (!vs.IsValid() || !fs.IsValid()) { fprintf(stderr, "Shader creation failed\n"); return 1; }

    // Pipeline
    rhi::PipelineDesc pipeDesc;
    pipeDesc.VertexShader = vs;
    pipeDesc.FragmentShader = fs;
    pipeDesc.RenderPass = renderer->GetDefaultRenderPass();

    pipeDesc.VertexLayout = {
        {0, rhi::Format::RGB32_FLOAT, 0,  0, sizeof(Vertex)},
        {1, rhi::Format::RGB32_FLOAT, 12, 0, sizeof(Vertex)},
    };

    // Disable face culling: triangle winding is CW in NDC, which
    // appears front-facing in Vulkan (Y-down viewport) but back-facing
    // in OpenGL (Y-up viewport). CullMode::None works for both.
    pipeDesc.Raster.Cull = rhi::RasterState::CullMode::None;

    pipeDesc.BufferSlots = {
        {0, rhi::BufferSlot::Type::Uniform,
         rhi::ShaderStage::Vertex | rhi::ShaderStage::Fragment, sizeof(UniformBufferObject)},
    };

    rhi::PipelineHandle pipeline = device.CreatePipeline(pipeDesc);
    if (!pipeline.IsValid()) { fprintf(stderr, "Pipeline creation failed\n"); return 1; }

    // Geometry
    rhi::GeometryDesc geoDesc;
    geoDesc.VertexBuffers[0] = vb;
    geoDesc.VertexBufferCount = 1;
    geoDesc.VertexCount = 3;
    rhi::GeometryHandle geometry = device.CreateGeometry(geoDesc);

    printf("Resources created.\n");

    // ---- Default swapchain resources ----
    rhi::RenderPassHandle  defaultRP = renderer->GetDefaultRenderPass();
    rhi::FramebufferHandle defaultFB = renderer->GetDefaultFramebuffer();

    // ---- Main Loop ----
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        surface.UpdateSize();
        auto sd = surface.GetDesc();
        if (sd.Width == 0 || sd.Height == 0) continue;

        renderer->BeginFrame();
        rhi::ICommandBuffer& cmd = renderer->GetCommandBuffer();

        rhi::Rect2D area = {{0, 0}, {sd.Width, sd.Height}};

        rhi::ClearValue cv;
        cv.Color.RGBA = {0.1f, 0.1f, 0.1f, 1.0f};

        cmd.BeginRenderPass(defaultRP, defaultFB, area, &cv, 1);
        cmd.BindPipeline(pipeline);
        cmd.BindGeometry(geometry);
        cmd.BindUniformBuffer(0, ub, 0, sizeof(UniformBufferObject));
        cmd.Draw(3);
        cmd.EndRenderPass();

        renderer->EndFrame();
    }

    renderer.reset();
    glfwDestroyWindow(window);
    glfwTerminate();
    printf("Done.\n");
    return 0;
}
