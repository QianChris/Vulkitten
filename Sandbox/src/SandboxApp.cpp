#include <Vulkitten.h>
#include "imgui.h"

#include <filesystem>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "Platform/OpenGL/OpenGLShader.h"
#include "Vulkitten/Renderer/CameraController.h"

class ExampleLayer : public Vulkitten::Layer
{
public:
    ExampleLayer() : Layer("Example")
    , m_CameraController(1280. / 720.)
    {
        auto currPath = std::filesystem::current_path().string();
        VKT_INFO("Current path is {0}", currPath);

        Vulkitten::FileSystem::RegisterPath("../../Sandbox", "sandbox");

        m_ShaderLibrary.Load("sandbox://assets/shaders/FlatColor.shader");
        m_ShaderLibrary.Load("sandbox://assets/shaders/SolidColor.shader");
        m_ShaderLibrary.Load("sandbox://assets/shaders/Texture.shader");

        m_VAO = Vulkitten::VertexArray::Create();
        {
            float vertices[3 * 7] = {
                -0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f,1.0f,
                0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 0.0f,1.0f,
                0.0f,  0.5f, 0.0f, 0.0f, 0.0f, 1.0f,1.0f,
            };
            Vulkitten::Ref<Vulkitten::VertexBuffer> vertexBuffer;
            vertexBuffer = Vulkitten::VertexBuffer::Create(vertices, sizeof(vertices));

            unsigned int indices[3] = { 0, 1, 2 };
            Vulkitten::Ref<Vulkitten::IndexBuffer> indexBuffer;
            indexBuffer = Vulkitten::IndexBuffer::Create(indices, sizeof(indices) / sizeof(uint32_t));

            Vulkitten::BufferLayout layout = {
                { Vulkitten::ShaderDataType::Float3, "a_Position" },
                { Vulkitten::ShaderDataType::Float4, "a_Color" },
            };
            vertexBuffer->SetLayout(layout);
            m_VAO->AddVertexBuffer(vertexBuffer);
            m_VAO->SetIndexBuffer(indexBuffer);
        }
        
        m_SquareVAO = Vulkitten::VertexArray::Create();
        {
            float vertices[4 * 5] = {
                -0.75f, -0.75f, 0.0f, 0.0f, 0.0f,
                0.75f, -0.75f, 0.0f, 1.0f, 0.0f,
                0.75f,  0.75f, 0.0f, 1.0f, 1.0f,
                -0.75f,  0.75f, 0.0f, 0.0f, 1.0f,
            };
            Vulkitten::Ref<Vulkitten::VertexBuffer> vertexBuffer;
            vertexBuffer = Vulkitten::VertexBuffer::Create(vertices, sizeof(vertices));

            unsigned int indices[6] = { 0, 1, 2, 2, 3, 0 };
            Vulkitten::Ref<Vulkitten::IndexBuffer> indexBuffer;
            indexBuffer = Vulkitten::IndexBuffer::Create(indices, sizeof(indices) / sizeof(uint32_t));

            Vulkitten::BufferLayout layout = {
                { Vulkitten::ShaderDataType::Float3, "a_Position" },
				{ Vulkitten::ShaderDataType::Float2, "a_TexCoord" },
            };
            vertexBuffer->SetLayout(layout);
            m_SquareVAO->AddVertexBuffer(vertexBuffer);
            m_SquareVAO->SetIndexBuffer(indexBuffer);
        }

        m_Texture = Vulkitten::Texture2D::Create("sandbox://assets/textures/Checkerboard.png");
        m_LogoTexture = Vulkitten::Texture2D::Create("sandbox://assets/textures/ChernoLogo.png");

		auto textureShader = m_ShaderLibrary.Get("Texture");
        std::dynamic_pointer_cast<Vulkitten::OpenGLShader>(textureShader)->Bind();
        std::dynamic_pointer_cast<Vulkitten::OpenGLShader>(textureShader)->UploadUniformInt("u_Texture", 0);
    }

    void OnUpdate(Vulkitten::Timestep timestep) override
    {
        Vulkitten::RenderCommand::SetClearColor({ 0.1f, 0.1f, 0.1f, 1.0f });
        Vulkitten::RenderCommand::Clear();

        m_CameraController.OnUpdate(timestep);

        Vulkitten::Renderer::BeginScene(m_CameraController.GetCamera());

		glm::mat4 scale = glm::scale(glm::mat4(1.0f), glm::vec3(0.1f));
        auto squareShader = m_ShaderLibrary.Get("SolidColor");
        for (int i =0; i<10; i++){
            for (int j =0; j<20; j++){
                glm::vec3 pos{ i * 0.21f, j * 0.21f, 0.0f };
                pos += m_SquarePosition;
                glm::mat4 transform = glm::translate(glm::mat4(1.0f), pos) * scale;
                if ((i + j) % 2 == 0){
                    squareShader->Bind();
                    std::dynamic_pointer_cast<Vulkitten::OpenGLShader>(squareShader)
                        ->UploadUniformFloat4("u_Color", glm::vec4(m_RedColor, 1.f));
                } else {
                    squareShader->Bind();
                    std::dynamic_pointer_cast<Vulkitten::OpenGLShader>(squareShader)
                        ->UploadUniformFloat4("u_Color", glm::vec4(m_BlueColor, 1.f));
				}

				Vulkitten::Renderer::Submit(squareShader, m_SquareVAO, transform);
            }
		}
        //Vulkitten::Renderer::Submit(m_SquareShader, m_SquareVAO, glm::translate(glm::mat4(1.0f), m_SquarePosition));

		auto textureShader = m_ShaderLibrary.Get("Texture");

        m_Texture->Bind();
        Vulkitten::Renderer::Submit(textureShader, m_SquareVAO, glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f)));

        m_LogoTexture->Bind();
        Vulkitten::Renderer::Submit(textureShader, m_SquareVAO, glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f)));

        // Triangle
        //Vulkitten::Renderer::Submit(m_Shader, m_VAO);

        Vulkitten::Renderer::EndScene();

        if (Vulkitten::Input::IsKeyPressed(VKT_KEY_UP)){
            m_SquarePosition.y += m_SquareMoveSpeed * timestep;
        } else if (Vulkitten::Input::IsKeyPressed(VKT_KEY_DOWN)){
            m_SquarePosition.y -= m_SquareMoveSpeed * timestep;
        } else if (Vulkitten::Input::IsKeyPressed(VKT_KEY_LEFT)){
            m_SquarePosition.x -= m_SquareMoveSpeed * timestep;
        } else if (Vulkitten::Input::IsKeyPressed(VKT_KEY_RIGHT)){
            m_SquarePosition.x += m_SquareMoveSpeed * timestep;
		}

        //VKT_INFO("timestep = {}", timestep.GetSeconds());
    }

    virtual void OnImguiRender() override
    {
        ImGui::Begin("Test");
		ImGui::ColorEdit3("Red Color", glm::value_ptr(m_RedColor));
		ImGui::ColorEdit3("Blue Color", glm::value_ptr(m_BlueColor));
        ImGui::End();
	}

    void OnEvent(Vulkitten::Event& event) override
    {
        m_CameraController.OnEvent(event);
    }

private:
    Vulkitten::ShaderLibrary m_ShaderLibrary;

    Vulkitten::Ref<Vulkitten::VertexArray> m_VAO;
    Vulkitten::Ref<Vulkitten::VertexArray> m_SquareVAO;

    Vulkitten::Ref<Vulkitten::Texture2D> m_Texture;
    Vulkitten::Ref<Vulkitten::Texture2D> m_LogoTexture;

    Vulkitten::CameraController m_CameraController;

	glm::vec3 m_SquarePosition{ 0.0f, 0.0f, 0.0f };
    float m_SquareMoveSpeed = 1.0f;
    glm::vec3 m_RedColor{ 0.8f, 0.2f, 0.3f };
    glm::vec3 m_BlueColor{ 0.2f, 0.3f, 0.8f };
};

class Sandbox : public Vulkitten::Application {
public:
    Sandbox() {
        PushLayer(new ExampleLayer());
    }
    ~Sandbox() {}
};

Vulkitten::Application* Vulkitten::CreateApplication() {
    return new Sandbox();
}
