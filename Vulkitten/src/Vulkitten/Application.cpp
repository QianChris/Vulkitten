#include "vktpch.h"
#include "Vulkitten/Application.h"

#include <glad/glad.h>

#include "Vulkitten/Layer.h"
#include "Vulkitten/Input.h"

#include <glm/glm.hpp>

namespace Vulkitten
{
    Application* Application::s_Instance = nullptr;

    static GLenum ShaderDataTypeToOpenGLBaseType(ShaderDataType type)
    {
        switch (type)
        {
            case ShaderDataType::Float:   return GL_FLOAT;
            case ShaderDataType::Float2:  return GL_FLOAT;
            case ShaderDataType::Float3:  return GL_FLOAT;
            case ShaderDataType::Float4:  return GL_FLOAT;
            case ShaderDataType::Mat3:    return GL_FLOAT;
            case ShaderDataType::Mat4:    return GL_FLOAT;
            case ShaderDataType::Int:     return GL_INT;
            case ShaderDataType::Int2:    return GL_INT;
            case ShaderDataType::Int3:    return GL_INT;
            case ShaderDataType::Int4:    return GL_INT;
            case ShaderDataType::Uint:    return GL_UNSIGNED_INT;
            case ShaderDataType::Uint2:   return GL_UNSIGNED_INT;
            case ShaderDataType::Uint3:   return GL_UNSIGNED_INT;
            case ShaderDataType::Uint4:   return GL_UNSIGNED_INT;
            case ShaderDataType::Bool:    return GL_BOOL;
        }

        VKT_CORE_ASSERT(false, "Unknown ShaderDataType!");
        return 0;
    }

    Application::Application()
    {
        VKT_ASSERT(!s_Instance, "Application already exists!");
        s_Instance = this;

        m_Window = std::unique_ptr<Window>(Window::Create());
        m_Window->SetEventCallback(VKT_BIND_EVENT_FN(Application::OnEvent));

        m_ImGuiLayer = new ImGuiLayer();
        PushOverlay(m_ImGuiLayer);

        // TEMPORARY: GL TRIANGLE
        glGenVertexArrays(1, &m_VertexArray);
        glBindVertexArray(m_VertexArray);

        float vertices[3 * 7] = {
            -0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f,1.0f,
             0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 0.0f,1.0f,
             0.0f,  0.5f, 0.0f, 0.0f, 0.0f, 1.0f,1.0f,
        };
        m_VBO.reset(VertexBuffer::Create(vertices, sizeof(vertices)));

        unsigned int indices[3] = { 0, 1, 2 };
        m_IBO.reset(IndexBuffer::Create(indices, sizeof(indices) / sizeof(uint32_t)));

        {
            BufferLayout layout = {
                { ShaderDataType::Float3, "a_Position" },
                { ShaderDataType::Float4, "a_Color" },
            };
            m_VBO->SetLayout(layout);
        }

        uint32_t index = 0;
        for (auto& element : m_VBO->GetLayout().GetElements())
        {
            glEnableVertexAttribArray(index);
            glVertexAttribPointer(index,
                element.GetComponentCount(),
                ShaderDataTypeToOpenGLBaseType(element.Type),
                element.Normalized ? GL_TRUE : GL_FALSE,
                m_VBO->GetLayout().GetStride(),
                (const void*)element.Offset);
            index++;
        }

        
        std::string vertexSrc = R"(
            #version 330 core

            layout(location = 0) in vec3 a_Position;
            layout(location = 1) in vec4 a_Color;
            out vec4 v_Color;

            void main()
            {
                v_Color = a_Color;
                gl_Position = vec4(a_Position, 1.0);
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

        m_Shader = std::make_unique<Shader>(vertexSrc, fragmentSrc);
    }

    Application::~Application()
    {
    }

    void Application::Run()
    {
        while (m_Running)
        {
            glClearColor(0.1f, 0.1f, 0.1f, 1);
            glClear(GL_COLOR_BUFFER_BIT);

			m_Shader->Bind();
            m_VBO->Bind();
            m_IBO->Bind();
            glDrawElements(GL_TRIANGLES, m_IBO->GetCount(), GL_UNSIGNED_INT, nullptr);
			m_Shader->Unbind();

            for (Layer *layer : m_LayerStack)
                layer->OnUpdate();

            m_ImGuiLayer->Begin();
            for (Layer *layer : m_LayerStack)
                layer->OnImguiRender();
            m_ImGuiLayer->End();

            m_Window->OnUpdate();

            //auto [mouseX, mouseY] = Input::GetMousePosition();
            //VKT_CORE_TRACE("{0}, {1}", mouseX, mouseY);
        }
    }

    void Application::OnEvent(Event &e)
    {
        EventDispatcher dispatcher(e);
        dispatcher.Dispatch<WindowCloseEvent>(VKT_BIND_EVENT_FN(Application::OnWindowClose));

        for (auto it = m_LayerStack.end(); it != m_LayerStack.begin();)
        {
            (*--it)->OnEvent(e);
            if (e.Handled)
                break;
        }

        //VKT_CORE_TRACE("{}", e.ToString());
    }

    void Application::PushLayer(Layer *layer)
    {
        m_LayerStack.PushLayer(layer);
        layer->OnAttach();
    }

    void Application::PushOverlay(Layer *overlay)
    {
        m_LayerStack.PushOverlay(overlay);
        overlay->OnAttach();
    }

    bool Application::OnWindowClose(WindowCloseEvent &e)
    {
        m_Running = false;
        return false;
    }
}