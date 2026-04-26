#include <Vulkitten.h>
#include "imgui.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "Platform/OpenGL/OpenGLShader.h"

class ExampleLayer : public Vulkitten::Layer
{
public:
    ExampleLayer() : Layer("Example")
    , m_Camera(-1.6f, 1.6f, -0.9f, 0.9f)
    {
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
        
        std::string vertexSrc = R"(
            #version 330 core

            layout(location = 0) in vec3 a_Position;
            layout(location = 1) in vec4 a_Color;

            uniform mat4 u_ViewProjection;
            uniform mat4 u_Transform;

            out vec4 v_Color;

            void main()
            {
                v_Color = a_Color;
                gl_Position = u_ViewProjection * u_Transform * vec4(a_Position, 1.0);
            }
        )"; 
        std::string fragmentSrc = R"(
            #version 330 core

            layout(location = 0) out vec4 color;
            in vec4 v_Color;

            void main()
            {
                color = v_Color;
            }
        )";
        m_Shader = Vulkitten::Shader::Create(vertexSrc, fragmentSrc);

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

        std::string squareVertexSrc = R"(
            #version 330 core

            layout(location = 0) in vec3 a_Position;
            uniform mat4 u_ViewProjection;
            uniform mat4 u_Transform;

            void main()
            {
                gl_Position = u_ViewProjection * u_Transform * vec4(a_Position, 1.0);
            }
        )";

        std::string squareFragmentSrc = R"(
            #version 330 core

            layout(location = 0) out vec4 color;
            uniform vec4 u_Color;

            void main()
            {
                color = u_Color;
            }
        )";
        m_SquareShader = Vulkitten::Shader::Create(squareVertexSrc, squareFragmentSrc);

        std::string textureVertexSrc = R"(
            #version 330 core

            layout(location = 0) in vec3 a_Position;
            layout(location = 1) in vec2 a_TexCoord;
            uniform mat4 u_ViewProjection;
            uniform mat4 u_Transform;

            out vec2 v_TexCoord;

            void main()
            {
                v_TexCoord = a_TexCoord;
                gl_Position = u_ViewProjection * u_Transform * vec4(a_Position, 1.0);
            }
        )";
        std::string textureFragmentSrc = R"(
            #version 330 core

            layout(location = 0) out vec4 color;
            uniform sampler2D u_Texture;
            in vec2 v_TexCoord;

            void main()
            {
                color = texture(u_Texture, v_TexCoord);
                // color = vec4(v_TexCoord, 0.0, 1.0);
            }
        )";
        m_TextureShader = Vulkitten::Shader::Create(textureVertexSrc, textureFragmentSrc);

        m_Texture = Vulkitten::Texture2D::Create("../../Sandbox/assets/textures/Checkerboard.png");
        m_LogoTexture = Vulkitten::Texture2D::Create("../../Sandbox/assets/textures/ChernoLogo.png");

        std::dynamic_pointer_cast<Vulkitten::OpenGLShader>(m_TextureShader)->Bind();
        std::dynamic_pointer_cast<Vulkitten::OpenGLShader>(m_TextureShader)->UploadUniformInt("u_Texture", 0);
    }

    void OnUpdate(Vulkitten::Timestep timestep) override
    {
        Vulkitten::RenderCommand::SetClearColor({ 0.1f, 0.1f, 0.1f, 1.0f });
        Vulkitten::RenderCommand::Clear();

        //m_Camera.SetRotation(m_Camera.GetRotation() + 1.f);
        m_Camera.SetRotation(m_CameraRotation); // degree
        m_Camera.SetPosition(m_CameraPosition);

        Vulkitten::Renderer::BeginScene(m_Camera);

		glm::mat4 scale = glm::scale(glm::mat4(1.0f), glm::vec3(0.1f));
        for (int i =0; i<10; i++){
            for (int j =0; j<20; j++){
                glm::vec3 pos{ i * 0.21f, j * 0.21f, 0.0f };
                pos += m_SquarePosition;
                glm::mat4 transform = glm::translate(glm::mat4(1.0f), pos) * scale;
                if ((i + j) % 2 == 0){
                    m_SquareShader->Bind();
                    std::dynamic_pointer_cast<Vulkitten::OpenGLShader>(m_SquareShader)
                        ->UploadUniformFloat4("u_Color", glm::vec4(m_RedColor, 1.f));
                } else {
                    m_SquareShader->Bind();
                    std::dynamic_pointer_cast<Vulkitten::OpenGLShader>(m_SquareShader)
                        ->UploadUniformFloat4("u_Color", glm::vec4(m_BlueColor, 1.f));
				}

				Vulkitten::Renderer::Submit(m_SquareShader, m_SquareVAO, transform);
            }
		}
        //Vulkitten::Renderer::Submit(m_SquareShader, m_SquareVAO, glm::translate(glm::mat4(1.0f), m_SquarePosition));

        m_Texture->Bind();
        Vulkitten::Renderer::Submit(m_TextureShader, m_SquareVAO, glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f)));

        m_LogoTexture->Bind();
        Vulkitten::Renderer::Submit(m_TextureShader, m_SquareVAO, glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f)));

        // Triangle
        //Vulkitten::Renderer::Submit(m_Shader, m_VAO);

        Vulkitten::Renderer::EndScene();

        if (Vulkitten::Input::IsKeyPressed(VKT_KEY_UP)){
            m_CameraPosition.y += m_CameraMoveSpeed * timestep;
        } else if (Vulkitten::Input::IsKeyPressed(VKT_KEY_DOWN)){
            m_CameraPosition.y -= m_CameraMoveSpeed * timestep;
        } else if (Vulkitten::Input::IsKeyPressed(VKT_KEY_LEFT)){
            m_CameraPosition.x -= m_CameraMoveSpeed * timestep;
        } else if (Vulkitten::Input::IsKeyPressed(VKT_KEY_RIGHT)){
            m_CameraPosition.x += m_CameraMoveSpeed * timestep;
        }

        if (Vulkitten::Input::IsKeyPressed(VKT_KEY_Q)){
            m_CameraRotation += m_CameraRotationSpeed * timestep;
        } else if (Vulkitten::Input::IsKeyPressed(VKT_KEY_E)){
            m_CameraRotation -= m_CameraRotationSpeed * timestep;
        }

        if (Vulkitten::Input::IsKeyPressed(VKT_KEY_W)){
            m_SquarePosition.y += m_CameraMoveSpeed * timestep;
        } else if (Vulkitten::Input::IsKeyPressed(VKT_KEY_S)){
            m_SquarePosition.y -= m_CameraMoveSpeed * timestep;
        } else if (Vulkitten::Input::IsKeyPressed(VKT_KEY_A)){
            m_SquarePosition.x -= m_CameraMoveSpeed * timestep;
        } else if (Vulkitten::Input::IsKeyPressed(VKT_KEY_D)){
            m_SquarePosition.x += m_CameraMoveSpeed * timestep;
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
        // Log key code
        //if (event.GetEventType() == Vulkitten::EventType::KeyPressed)
        //{
            //Vulkitten::KeyPressedEvent& e = (Vulkitten::KeyPressedEvent&)event;
            //VKT_TRACE("{0}", (char) e.GetKeyCode());
		//}
    }

private:
    Vulkitten::Ref<Vulkitten::Shader> m_Shader;
    Vulkitten::Ref<Vulkitten::VertexArray> m_VAO;

    Vulkitten::Ref<Vulkitten::Shader> m_SquareShader;
    Vulkitten::Ref<Vulkitten::VertexArray> m_SquareVAO;

    Vulkitten::Ref<Vulkitten::Shader> m_TextureShader;
    Vulkitten::Ref<Vulkitten::Texture2D> m_Texture;
    Vulkitten::Ref<Vulkitten::Texture2D> m_LogoTexture;

    Vulkitten::OrthographicCamera m_Camera;

    glm::vec3 m_CameraPosition{ 0.0f, 0.0f, 0.0f };
    float m_CameraRotation = 0.0f;
    float m_CameraMoveSpeed = 1.0f, m_CameraRotationSpeed = 90.f; // degree per second

	glm::vec3 m_SquarePosition{ 0.0f, 0.0f, 0.0f };
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
