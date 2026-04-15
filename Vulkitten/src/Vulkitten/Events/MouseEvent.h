#pragma once

#include "Vulkitten/Events/Event.h"

namespace Vulkitten
{

class VKT_API MouseButtonEvent : public Event
{
public:
    int GetMouseButton() const { return m_MouseButton; }

    EVENT_CLASS_CATEGORY(EventCategoryMouse | EventCategoryMouseButton | EventCategoryInput)

protected:
    MouseButtonEvent(int button)
        : m_MouseButton(button) {}

    int m_MouseButton;
};

class VKT_API MouseButtonPressedEvent : public MouseButtonEvent
{
public:
    MouseButtonPressedEvent(int button)
        : MouseButtonEvent(button) {}

    std::string ToString() const override
    {
        return GetName() + std::string(": ") + std::to_string(m_MouseButton);
    }

    EVENT_CLASS_TYPE(MouseButtonPressed)
};

class VKT_API MouseButtonReleasedEvent : public MouseButtonEvent
{
public:
    MouseButtonReleasedEvent(int button)
        : MouseButtonEvent(button) {}

    std::string ToString() const override
    {
        return GetName() + std::string(": ") + std::to_string(m_MouseButton);
    }

    EVENT_CLASS_TYPE(MouseButtonReleased)
};

class VKT_API MouseMovedEvent : public Event
{
public:
    MouseMovedEvent(float x, float y)
        : m_MouseX(x), m_MouseY(y) {}

    float GetX() const { return m_MouseX; }
    float GetY() const { return m_MouseY; }

    std::string ToString() const override
    {
        return GetName() + std::string(": (") + std::to_string(m_MouseX) + ", " + std::to_string(m_MouseY) + ")";
    }

    EVENT_CLASS_TYPE(MouseMoved)
    EVENT_CLASS_CATEGORY(EventCategoryMouse | EventCategoryInput)

private:
    float m_MouseX;
    float m_MouseY;
};

class VKT_API MouseScrolledEvent : public Event
{
public:
    MouseScrolledEvent(float xOffset, float yOffset)
        : m_XOffset(xOffset), m_YOffset(yOffset) {}

    float GetXOffset() const { return m_XOffset; }
    float GetYOffset() const { return m_YOffset; }

    std::string ToString() const override
    {
        return GetName() + std::string(": (") + std::to_string(m_XOffset) + ", " + std::to_string(m_YOffset) + ")";
    }

    EVENT_CLASS_TYPE(MouseScrolled)
    EVENT_CLASS_CATEGORY(EventCategoryMouse | EventCategoryInput)

private:
    float m_XOffset;
    float m_YOffset;
};

}
