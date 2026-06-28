#pragma once

#include <Vulkitten.h>
#include "imgui.h"

#include <glad/glad.h>

// ============================================================
// ExampleLayer3D - 3D glTF asset visualization demo.
//
// Loads a glTF 2.0 file using GltfLoader, creates GPU buffers,
// and renders the mesh with a simple directional-light shader
// and orbit camera.
// ============================================================

class ExampleLayer3D : public Vulkitten::Layer
{
public:
    ExampleLayer3D(const std::string& gltfPath);
    ~ExampleLayer3D();

    void OnAttach() override;
    void OnDetach() override;
    void OnUpdate(Vulkitten::Timestep timestep, Vulkitten::SceneContext& ctx) override;
    void OnImguiRender() override;
    void OnEvent(Vulkitten::Event& event) override;

private:
    struct GpuMesh
    {
        GLuint VAO = 0;
        GLuint VBO = 0;
        GLuint EBO = 0;
        uint32_t IndexCount = 0;
        std::string Name;
    };

    void LoadModel(const std::string& path);
    void CreateGpuResources();
    void DestroyGpuResources();
    void UpdateCamera(float dt);

    // Camera state
    glm::vec3 m_CameraPos{0.0f, 2.0f, 5.0f};
    glm::vec3 m_CameraTarget{0.0f, 0.0f, 0.0f};
    glm::vec3 m_CameraUp{0.0f, 1.0f, 0.0f};
    float m_OrbitAngleX = 0.3f;
    float m_OrbitAngleY = 0.0f;
    float m_OrbitDistance = 5.0f;
    float m_FOV = 45.0f;

    // Input state
    bool m_RightMouseDown = false;
    float m_LastMouseX = 0.0f;
    float m_LastMouseY = 0.0f;

    // GPU resources
    GLuint m_ShaderProgram = 0;
    GLuint m_CameraUBO = 0;
    std::vector<GpuMesh> m_Meshes;

    // glTF data
    Vulkitten::Scope<Vulkitten::GltfLoader> m_Loader;

    // Shader uniform locations
    GLint m_LocModel = -1;
    GLint m_LocLightDir = -1;
    GLint m_LocBaseColor = -1;
};
