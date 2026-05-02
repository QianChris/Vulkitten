#include "vktpch.h"
#include "OpenGLVertexArray.h"

#include "OpenGLUtil.h"

#include <glad/glad.h>

#include "Vulkitten/Perf/Instrumentor.h"

namespace Vulkitten {
    OpenGLVertexArray::OpenGLVertexArray()
    {
        VKT_PROFILE_RENDER_FUNCTION();

        glGenVertexArrays(1, &m_RendererID);
    }

    OpenGLVertexArray::~OpenGLVertexArray()
    {
        VKT_PROFILE_RENDER_FUNCTION();

        glDeleteVertexArrays(1, &m_RendererID);
    }

    void OpenGLVertexArray::Bind() const
    {
        VKT_PROFILE_RENDER_FUNCTION();

        glBindVertexArray(m_RendererID);
    }

    void OpenGLVertexArray::Unbind() const
    {
        VKT_PROFILE_RENDER_FUNCTION();

        glBindVertexArray(0);
    }

    void OpenGLVertexArray::AddVertexBuffer(Ref<VertexBuffer> vertexBuffer)
    {
        VKT_PROFILE_RENDER_FUNCTION();

        VKT_CORE_ASSERT(vertexBuffer->GetLayout().GetElements().size(), "Vertex Buffer has no layout!");

        glBindVertexArray(m_RendererID);
        vertexBuffer->Bind();

        uint32_t index = 0;
        const auto& layout = vertexBuffer->GetLayout();
        for (const auto& element : layout.GetElements())
        {
            glEnableVertexAttribArray(index);
            glVertexAttribPointer(index,
                element.GetComponentCount(),
                OpenGLUtil::ShaderDataTypeToOpenGLBaseType(element.Type),
                element.Normalized ? GL_TRUE : GL_FALSE,
                layout.GetStride(),
                 (const void*)element.Offset);
            index++;
        }

        m_VertexBuffers.push_back(vertexBuffer);
    }

    void OpenGLVertexArray::SetIndexBuffer(Ref<IndexBuffer> indexBuffer)
    {
        VKT_PROFILE_RENDER_FUNCTION();

        glBindVertexArray(m_RendererID);
        indexBuffer->Bind();

        m_IndexBuffer = indexBuffer;
    }
}