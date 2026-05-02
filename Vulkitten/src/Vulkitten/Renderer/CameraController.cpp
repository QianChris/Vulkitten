#include "vktpch.h"
#include "CameraController.h"

#include "Vulkitten/Core/Input.h"
#include "Vulkitten/Core/KeyCode.h"
#include "Vulkitten/Core/Timestep.h"
#include "Vulkitten/Events/ApplicationEvent.h"

#include "Vulkitten/Perf/Instrumentor.h"

#include <glm/gtc/matrix_transform.hpp>

namespace Vulkitten {

    CameraController::CameraController(float aspectRatio, bool enableRotation)
        : m_Camera(-aspectRatio, aspectRatio, -1.0f, 1.0f)
        , m_AspectRatio(aspectRatio)
        , m_EnableRotation(enableRotation)
    {
    }

    void CameraController::OnUpdate(Timestep ts)
    {
        VKT_PROFILE_FUNCTION();

        if (Vulkitten::Input::IsKeyPressed(VKT_KEY_W))
        {
            m_Camera.SetPosition(m_Camera.GetPosition() + glm::vec3(0.0f, m_CameraTranslationSpeed * ts, 0.0f));
        }
        else if (Vulkitten::Input::IsKeyPressed(VKT_KEY_S))
        {
            m_Camera.SetPosition(m_Camera.GetPosition() + glm::vec3(0.0f, -m_CameraTranslationSpeed * ts, 0.0f));
        }

        if (Vulkitten::Input::IsKeyPressed(VKT_KEY_A))
        {
            m_Camera.SetPosition(m_Camera.GetPosition() + glm::vec3(-m_CameraTranslationSpeed * ts, 0.0f, 0.0f));
        }
        else if (Vulkitten::Input::IsKeyPressed(VKT_KEY_D))
        {
            m_Camera.SetPosition(m_Camera.GetPosition() + glm::vec3(m_CameraTranslationSpeed * ts, 0.0f, 0.0f));
        }

        if (m_EnableRotation)
        {
            if (Vulkitten::Input::IsKeyPressed(VKT_KEY_Q))
            {
                m_Camera.SetRotation(m_Camera.GetRotation() + m_CameraRotationSpeed * ts);
            }
            else if (Vulkitten::Input::IsKeyPressed(VKT_KEY_E))
            {
                m_Camera.SetRotation(m_Camera.GetRotation() - m_CameraRotationSpeed * ts);
            }
        }
    }

    bool CameraController::OnEvent(Event& e)
    {
        VKT_PROFILE_FUNCTION();

        EventDispatcher dispatcher(e);
        dispatcher.Dispatch<Vulkitten::MouseScrolledEvent>(VKT_BIND_EVENT_FN(CameraController::OnMouseScrolled));
        dispatcher.Dispatch<Vulkitten::WindowResizeEvent>(VKT_BIND_EVENT_FN(CameraController::OnWindowResized));
        return e.Handled;
    }

    bool CameraController::OnMouseScrolled(MouseScrolledEvent& e)
    {
        VKT_PROFILE_FUNCTION();

        m_ZoomLevel -= e.GetYOffset() * 0.25f;
        m_ZoomLevel = std::max(m_ZoomLevel, 0.25f);
        RecalculateView();
        return false;
    }

    bool CameraController::OnWindowResized(WindowResizeEvent& e)
    {
        VKT_PROFILE_FUNCTION();

        m_AspectRatio = (float)e.GetWidth() / (float)e.GetHeight();
        RecalculateView();
        return false;
    }

    void CameraController::RecalculateView()
    {
        VKT_PROFILE_FUNCTION();

        float bounds = 1.0f * m_ZoomLevel;
        m_Camera = OrthographicCamera(-m_AspectRatio * bounds, m_AspectRatio * bounds, -bounds, bounds);
    }

}
