#include "vktpch.h"
#include "SceneCamera.h"

#include <glm/gtc/matrix_transform.hpp>

namespace Vulkitten {

void SceneCamera::SetOrthographicProjection(float left, float right, float bottom, float top)
    {
        m_ProjectionType = ProjectionType::Orthographic;
        m_OrthographicSize = top - bottom;
        m_AspectRatio = (right - left) / (top - bottom);
        RecalculateProjection();
    }

    void SceneCamera::SetPerspectiveProjection(float fovRadians, float nearClip, float farClip)
    {
        m_ProjectionType = ProjectionType::Perspective;
        m_PerspectiveFOV = fovRadians;
        m_PerspectiveNear = nearClip;
        m_PerspectiveFar = farClip;
        RecalculateProjection();
    }

    void SceneCamera::RecalculateProjection()
    {
        if (m_ProjectionType == ProjectionType::Perspective)
        {
            m_ProjectionMatrix = glm::perspective(m_PerspectiveFOV, m_AspectRatio, m_PerspectiveNear, m_PerspectiveFar);
        }
        else
        {
            float orthoLeft = -m_OrthographicSize * m_AspectRatio * 0.5f;
            float orthoRight = m_OrthographicSize * m_AspectRatio * 0.5f;
            float orthoBottom = -m_OrthographicSize * 0.5f;
            float orthoTop = m_OrthographicSize * 0.5f;

            m_ProjectionMatrix = glm::ortho(orthoLeft, orthoRight, orthoBottom, orthoTop, m_OrthographicNear, m_OrthographicFar);
        }
        RecalculateViewProjectionMatrix();
    }

    void SceneCamera::RecalculateViewProjectionMatrix()
    {
        m_ViewProjectionMatrix = m_ProjectionMatrix * m_ViewMatrix;
    }

}