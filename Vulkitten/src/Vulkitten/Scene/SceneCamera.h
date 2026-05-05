#pragma once

#include "Vulkitten/Core/Core.h"
#include "Vulkitten/Renderer/Camera.h"

namespace Vulkitten {

    class VKT_API SceneCamera : public Camera
    {
    public:
        enum class ProjectionType { Perspective = 0, Orthographic = 1 };

    public:
        SceneCamera() = default;
        ~SceneCamera() = default;

        void SetOrthographicProjection(float left, float right, float bottom, float top);

        float GetZoomLevel() const { return m_OrthographicSize; }
        void SetZoomLevel(float zoomLevel) { m_OrthographicSize = zoomLevel; RecalculateProjection(); }

        void SetViewMatrix(const glm::mat4& viewMatrix) { m_ViewMatrix = viewMatrix; RecalculateViewProjectionMatrix(); }
        const glm::mat4& GetViewMatrix() const override { return m_ViewMatrix; }
        const glm::mat4& GetViewProjectionMatrix() const override { return m_ViewProjectionMatrix; }

    private:
        void RecalculateProjection();
        void RecalculateViewProjectionMatrix();

    private:
        ProjectionType m_ProjectionType = ProjectionType::Orthographic;

        float m_PerspectiveFOV = glm::radians(45.0f);
        float m_PerspectiveNear = 0.01f, m_PerspectiveFar = 1000.0f;

        float m_OrthographicSize = 10.0f;
        float m_OrthographicNear = -1.0f, m_OrthographicFar = 1.0f;

        float m_AspectRatio = 0.0f;

        glm::mat4 m_ViewProjectionMatrix{ 1.0f };
    };

}