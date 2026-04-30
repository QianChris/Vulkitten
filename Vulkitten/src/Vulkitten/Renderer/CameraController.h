#pragma once

#include "Vulkitten/Core/Core.h"
#include "Vulkitten/Core/Timestep.h"
#include "Vulkitten/Renderer/OrthographicCamera.h"
#include "Vulkitten/Events/Event.h"
#include "Vulkitten/Events/ApplicationEvent.h"
#include "Vulkitten/Events/MouseEvent.h"

#include <glm/glm.hpp>

namespace Vulkitten {

    class VKT_API CameraController
    {
    public:
        CameraController(float aspectRatio, bool enableRotation = true);

        OrthographicCamera& GetCamera() { return m_Camera; }
        const OrthographicCamera& GetCamera() const { return m_Camera; }

        float GetZoomLevel() const { return m_ZoomLevel; }
        void SetZoomLevel(float zoomLevel) { m_ZoomLevel = zoomLevel; }

        bool IsRotationEnabled() const { return m_EnableRotation; }
        void SetRotationEnabled(bool enabled) { m_EnableRotation = enabled; }

        void SetRotation(float rotation) { m_Camera.SetRotation(rotation); }

        void OnUpdate(Timestep ts);
        bool OnEvent(Event& e);

    private:
        bool OnMouseScrolled(MouseScrolledEvent& e);
        bool OnWindowResized(WindowResizeEvent& e);
        void RecalculateView();

    private:
        OrthographicCamera m_Camera;
        float m_ZoomLevel = 1.0f;
        float m_AspectRatio;
        float m_CameraTranslationSpeed = 5.0f;
        float m_CameraRotationSpeed = 180.0f;
        bool m_EnableRotation;
    };

}
