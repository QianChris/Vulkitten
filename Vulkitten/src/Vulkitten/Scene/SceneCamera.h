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
        void SetPerspectiveProjection(float fovRadians, float nearClip, float farClip);

        void SetProjectionType(ProjectionType type) 
        { 
            if (m_AspectRatio == 0.0f) 
                m_AspectRatio = 16.0f / 9.0f; 
            m_ProjectionType = type; 
            RecalculateProjection(); 
        }
        ProjectionType GetProjectionType() const { return m_ProjectionType; }

        void SetPerspectiveFOV(float fovRadians) { m_PerspectiveFOV = fovRadians; RecalculateProjection(); }
        float GetPerspectiveFOV() const { return m_PerspectiveFOV; }

        void SetPerspectiveNear(float nearClip) { m_PerspectiveNear = nearClip; RecalculateProjection(); }
        float GetPerspectiveNear() const { return m_PerspectiveNear; }

        void SetPerspectiveFar(float farClip) { m_PerspectiveFar = farClip; RecalculateProjection(); }
        float GetPerspectiveFar() const { return m_PerspectiveFar; }

        void SetOrthographicSize(float size) { m_OrthographicSize = size; RecalculateProjection(); }
        float GetOrthographicSize() const { return m_OrthographicSize; }

        void SetOrthographicNear(float nearClip) { m_OrthographicNear = nearClip; RecalculateProjection(); }
        float GetOrthographicNear() const { return m_OrthographicNear; }

        void SetOrthographicFar(float farClip) { m_OrthographicFar = farClip; RecalculateProjection(); }
        float GetOrthographicFar() const { return m_OrthographicFar; }

        void SetAspectRatio(float aspectRatio) { m_AspectRatio = aspectRatio; RecalculateProjection(); }

        float GetZoomLevel() const { return m_OrthographicSize; }
        void SetZoomLevel(float zoomLevel) { m_OrthographicSize = zoomLevel; RecalculateProjection(); }

void SetTransform(const glm::mat4& transform) { m_Transform = transform; RecalculateViewProjectionMatrix(); }
        glm::mat4 GetViewProjectionMatrix() const override { return m_ViewProjectionMatrix; }

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

        glm::mat4 m_Transform{ 1.0f };
        glm::mat4 m_ViewProjectionMatrix{ 1.0f };
        bool m_Dirty = true;
    };

}