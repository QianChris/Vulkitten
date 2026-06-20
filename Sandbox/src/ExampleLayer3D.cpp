#include <Vulkitten.h>
#include "ExampleLayer3D.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// ============================================================
// Simple shader compile/link helpers
// ============================================================

static GLuint CompileShader(GLenum type, const char* source)
{
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        VKT_CORE_ERROR("Shader compile error ({0}): {1}",
                       type == GL_VERTEX_SHADER ? "vert" : "frag", infoLog);
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

static GLuint CreateShaderProgram(const char* vertSrc, const char* fragSrc)
{
    GLuint vs = CompileShader(GL_VERTEX_SHADER, vertSrc);
    GLuint fs = CompileShader(GL_FRAGMENT_SHADER, fragSrc);
    if (!vs || !fs)
        return 0;

    GLuint program = glCreateProgram();
    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);

    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success)
    {
        char infoLog[512];
        glGetProgramInfoLog(program, 512, nullptr, infoLog);
        VKT_CORE_ERROR("Shader link error: {0}", infoLog);
        glDeleteProgram(program);
        program = 0;
    }

    glDeleteShader(vs);
    glDeleteShader(fs);
    return program;
}

// ============================================================
// Embedded shader sources (self-contained, no file I/O needed)
// ============================================================

static const char* kVertexShader = R"(
#version 450 core
layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_Normal;
layout(location = 2) in vec2 a_TexCoord;
layout(std140, binding = 0) uniform CameraUBO { mat4 u_Projection; mat4 u_View; };
uniform mat4 u_Model;
out vec3 v_Normal;
out vec2 v_TexCoord;
void main()
{
    vec4 worldPos = u_Model * vec4(a_Position, 1.0);
    gl_Position = u_Projection * u_View * worldPos;
    v_Normal = mat3(u_Model) * a_Normal;
    v_TexCoord = a_TexCoord;
}
)";

static const char* kFragmentShader = R"(
#version 450 core
in vec3 v_Normal;
in vec2 v_TexCoord;
out vec4 o_Color;
uniform vec3 u_LightDir = vec3(0.5, 0.8, 0.6);
uniform vec4 u_BaseColor = vec4(0.7, 0.7, 0.7, 1.0);
void main()
{
    vec3 N = normalize(v_Normal);
    vec3 L = normalize(u_LightDir);
    float diffuse = max(dot(N, L), 0.0);
    float ambient = 0.15;
    float lighting = ambient + diffuse * 0.85;
    o_Color = u_BaseColor * lighting;
}
)";

// ============================================================
// ExampleLayer3D
// ============================================================

ExampleLayer3D::ExampleLayer3D(const std::string& gltfPath)
    : Layer("Example3D")
{
    m_Loader = Vulkitten::CreateScope<Vulkitten::GltfLoader>(
        Vulkitten::Engine::Get().GetFileSystem());

    LoadModel(gltfPath);
}

ExampleLayer3D::~ExampleLayer3D()
{
    DestroyGpuResources();
}

void ExampleLayer3D::OnAttach()
{
    if (!m_Meshes.empty())
        CreateGpuResources();
}

void ExampleLayer3D::OnDetach()
{
    DestroyGpuResources();
}

// ============================================================
// glTF Loading
// ============================================================

void ExampleLayer3D::LoadModel(const std::string& path)
{
    uint32_t meshCount = m_Loader->Load(path);
    if (meshCount == 0)
    {
        VKT_CORE_ERROR("ExampleLayer3D: failed to load {0} — {1}",
                       path, m_Loader->GetLastError());
        return;
    }

    VKT_CORE_INFO("ExampleLayer3D: loaded {0} mesh(es) from {1}", meshCount, path);
}

// ============================================================
// GPU Resource Creation
// ============================================================

void ExampleLayer3D::CreateGpuResources()
{
    // --- Compile shader from embedded sources ---
    m_ShaderProgram = CreateShaderProgram(kVertexShader, kFragmentShader);
    if (!m_ShaderProgram)
        return;

    m_LocModel       = glGetUniformLocation(m_ShaderProgram, "u_Model");
    m_LocLightDir    = glGetUniformLocation(m_ShaderProgram, "u_LightDir");
    m_LocBaseColor   = glGetUniformLocation(m_ShaderProgram, "u_BaseColor");

    // --- Create Camera UBO (std140: mat4 proj + mat4 view = 128 bytes) ---
    glGenBuffers(1, &m_CameraUBO);
    glBindBuffer(GL_UNIFORM_BUFFER, m_CameraUBO);
    glBufferData(GL_UNIFORM_BUFFER, 2 * sizeof(glm::mat4), nullptr, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    // --- Create GPU buffers for each mesh ---
    const auto& meshes = m_Loader->GetMeshes();
    m_Meshes.reserve(meshes.size());

    for (const auto& meshData : meshes)
    {
        GpuMesh gpuMesh;
        gpuMesh.Name = meshData.Name;
        gpuMesh.IndexCount = static_cast<uint32_t>(meshData.Indices.size());

        glGenVertexArrays(1, &gpuMesh.VAO);
        glBindVertexArray(gpuMesh.VAO);

        // VBO
        glGenBuffers(1, &gpuMesh.VBO);
        glBindBuffer(GL_ARRAY_BUFFER, gpuMesh.VBO);
        glBufferData(GL_ARRAY_BUFFER,
                     meshData.Vertices.size() * sizeof(Vulkitten::GltfVertex),
                     meshData.Vertices.data(),
                     GL_STATIC_DRAW);

        // Vertex attributes
        glEnableVertexAttribArray(0); // Position
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vulkitten::GltfVertex),
                              (void*)offsetof(Vulkitten::GltfVertex, Position));

        glEnableVertexAttribArray(1); // Normal
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vulkitten::GltfVertex),
                              (void*)offsetof(Vulkitten::GltfVertex, Normal));

        glEnableVertexAttribArray(2); // TexCoord
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vulkitten::GltfVertex),
                              (void*)offsetof(Vulkitten::GltfVertex, TexCoord));

        // EBO
        if (gpuMesh.IndexCount > 0)
        {
            glGenBuffers(1, &gpuMesh.EBO);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gpuMesh.EBO);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                         meshData.Indices.size() * sizeof(uint32_t),
                         meshData.Indices.data(),
                         GL_STATIC_DRAW);
        }

        glBindVertexArray(0);
        m_Meshes.push_back(gpuMesh);
    }

    VKT_CORE_INFO("ExampleLayer3D: created {0} GPU mesh(es)", m_Meshes.size());
}

void ExampleLayer3D::DestroyGpuResources()
{
    for (auto& m : m_Meshes)
    {
        if (m.VAO) glDeleteVertexArrays(1, &m.VAO);
        if (m.VBO) glDeleteBuffers(1, &m.VBO);
        if (m.EBO) glDeleteBuffers(1, &m.EBO);
    }
    m_Meshes.clear();

    if (m_ShaderProgram) glDeleteProgram(m_ShaderProgram);
    m_ShaderProgram = 0;

    if (m_CameraUBO) glDeleteBuffers(1, &m_CameraUBO);
    m_CameraUBO = 0;
}

// ============================================================
// Camera
// ============================================================

void ExampleLayer3D::UpdateCamera(float /*dt*/)
{
    float cx = m_CameraTarget.x + m_OrbitDistance * cos(m_OrbitAngleY) * sin(m_OrbitAngleX);
    float cy = m_CameraTarget.y + m_OrbitDistance * sin(m_OrbitAngleY);
    float cz = m_CameraTarget.z + m_OrbitDistance * cos(m_OrbitAngleY) * cos(m_OrbitAngleX);
    m_CameraPos = glm::vec3(cx, cy, cz);
}

// ============================================================
// Per-frame Update
// ============================================================

void ExampleLayer3D::OnUpdate(Vulkitten::Timestep timestep, Vulkitten::SceneContext& /*ctx*/)
{
    UpdateCamera(timestep.GetSeconds());

    if (m_Meshes.empty() || !m_ShaderProgram)
        return;

    auto& window = Vulkitten::Application::Get().GetWindow();
    float aspect = static_cast<float>(window.GetWidth()) / window.GetHeight();
    glm::mat4 projection = glm::perspective(glm::radians(m_FOV), aspect, 0.1f, 100.0f);
    glm::mat4 view = glm::lookAt(m_CameraPos, m_CameraTarget, m_CameraUp);

    // Upload camera UBO
    struct CameraUBO { glm::mat4 proj; glm::mat4 view; };
    CameraUBO camData = { projection, view };
    glBindBuffer(GL_UNIFORM_BUFFER, m_CameraUBO);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(CameraUBO), &camData);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    // Bind shader and camera
    glUseProgram(m_ShaderProgram);
    glBindBufferBase(GL_UNIFORM_BUFFER, 0, m_CameraUBO);

    // Set lighting uniforms
    glUniform3f(m_LocLightDir, 0.5f, 0.8f, 0.6f);
    glUniform4f(m_LocBaseColor, 0.75f, 0.75f, 0.8f, 1.0f);

    // Draw each mesh
    for (const auto& mesh : m_Meshes)
    {
        glm::mat4 model = glm::mat4(1.0f);
        glUniformMatrix4fv(m_LocModel, 1, GL_FALSE, glm::value_ptr(model));

        glBindVertexArray(mesh.VAO);
        if (mesh.IndexCount > 0)
            glDrawElements(GL_TRIANGLES, mesh.IndexCount, GL_UNSIGNED_INT, nullptr);
        else
            glDrawArrays(GL_TRIANGLES, 0, 0); // Shouldn't happen with valid glTF
        glBindVertexArray(0);
    }

    glUseProgram(0);
}

// ============================================================
// ImGui
// ============================================================

void ExampleLayer3D::OnImguiRender()
{
    ImGui::Begin("3D Viewer");

    ImGui::Text("Meshes: %zu", m_Loader->GetMeshes().size());
    for (const auto& mesh : m_Meshes)
        ImGui::Text("  %s — %u tris", mesh.Name.c_str(), mesh.IndexCount / 3);

    ImGui::Separator();
    ImGui::DragFloat("FOV", &m_FOV, 0.5f, 10.0f, 120.0f);
    ImGui::DragFloat("Distance", &m_OrbitDistance, 0.1f, 0.5f, 50.0f);
    ImGui::DragFloat("Orbit X", &m_OrbitAngleX, 0.01f);
    ImGui::DragFloat("Orbit Y", &m_OrbitAngleY, 0.01f, -1.5f, 1.5f);

    if (!m_Loader->GetLastError().empty())
    {
        ImGui::Separator();
        ImGui::TextColored({1, 0, 0, 1}, "Error: %s", m_Loader->GetLastError().c_str());
    }

    ImGui::End();
}

// ============================================================
// Input
// ============================================================

void ExampleLayer3D::OnEvent(Vulkitten::Event& event)
{
    Vulkitten::EventDispatcher dispatcher(event);

    dispatcher.Dispatch<Vulkitten::MouseButtonPressedEvent>(
        [this](Vulkitten::MouseButtonPressedEvent& e)
        {
            if (e.GetMouseButton() == VKT_MOUSE_BUTTON_RIGHT)
            {
                m_RightMouseDown = true;
                return true;
            }
            return false;
        });

    dispatcher.Dispatch<Vulkitten::MouseButtonReleasedEvent>(
        [this](Vulkitten::MouseButtonReleasedEvent& e)
        {
            if (e.GetMouseButton() == VKT_MOUSE_BUTTON_RIGHT)
            {
                m_RightMouseDown = false;
                return true;
            }
            return false;
        });

    dispatcher.Dispatch<Vulkitten::MouseMovedEvent>(
        [this](Vulkitten::MouseMovedEvent& e)
        {
            if (m_RightMouseDown)
            {
                float dx = e.GetX() - m_LastMouseX;
                float dy = e.GetY() - m_LastMouseY;
                m_OrbitAngleX += dx * 0.005f;
                m_OrbitAngleY += dy * 0.005f;
                if (m_OrbitAngleY > 1.5f)  m_OrbitAngleY = 1.5f;
                if (m_OrbitAngleY < -1.5f) m_OrbitAngleY = -1.5f;
            }
            m_LastMouseX = e.GetX();
            m_LastMouseY = e.GetY();
            return false;
        });

    dispatcher.Dispatch<Vulkitten::MouseScrolledEvent>(
        [this](Vulkitten::MouseScrolledEvent& e)
        {
            m_OrbitDistance -= e.GetYOffset() * 0.5f;
            if (m_OrbitDistance < 0.5f)  m_OrbitDistance = 0.5f;
            if (m_OrbitDistance > 50.0f) m_OrbitDistance = 50.0f;
            return true;
        });
}
