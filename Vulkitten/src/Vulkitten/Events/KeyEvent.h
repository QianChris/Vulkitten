#pragma once

#include "Vulkitten/Events/Event.h"

namespace Vulkitten
{

class VKT_API KeyEvent : public Event
{
public:
    int GetKeyCode() const { return m_KeyCode; }

    EVENT_CLASS_CATEGORY(EventCategoryKeyboard | EventCategoryInput)

protected:
    KeyEvent(int keyCode)
        : m_KeyCode(keyCode) {}

    int m_KeyCode;
};

class VKT_API KeyPressedEvent : public KeyEvent
{
public:
    KeyPressedEvent(int keyCode, int repeatCount)
        : KeyEvent(keyCode), m_RepeatCount(repeatCount) {}

    int GetRepeatCount() const { return m_RepeatCount; }

    std::string ToString() const override
    {
        return GetName() + std::string(": ") + std::to_string(m_KeyCode) + " (" + std::to_string(m_RepeatCount) + " repeats)";
    }

    EVENT_CLASS_TYPE(KeyPressed)

private:
    int m_RepeatCount;
};

class VKT_API KeyReleasedEvent : public KeyEvent
{
public:
    KeyReleasedEvent(int keyCode)
        : KeyEvent(keyCode) {}

    std::string ToString() const override
    {
        return GetName() + std::string(": ") + std::to_string(m_KeyCode);
    }

    EVENT_CLASS_TYPE(KeyReleased)
};

class VKT_API KeyTypedEvent : public KeyEvent
{
public:
    KeyTypedEvent(int keyCode)
        : KeyEvent(keyCode) {}

    std::string ToString() const override
    {
        return GetName() + std::string(": ") + std::to_string(m_KeyCode);
    }

    EVENT_CLASS_TYPE(KeyTyped)
};

}
