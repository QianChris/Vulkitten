#pragma once

#include "Vulkitten/Core/Core.h"
#include "Vulkitten/Renderer/Texture.h"
#include "Vulkitten/Scene/SceneCamera.h"
#include "Vulkitten/Scene/ScriptableEntity.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace Vulkitten {

    struct TagComponent
    {
        std::string Tag;

        operator std::string& () { return Tag; }
        operator const std::string& () const { return Tag; }
    };

    struct TransformComponent
    {
        TransformComponent() = default;
        TransformComponent(const TransformComponent&) = default;

        glm::vec3 Position{ 0.0f, 0.0f, 0.0f };
        glm::vec3 Rotation{ 0.0f, 0.0f, 0.0f };
        glm::vec3 Scale{ 1.0f, 1.0f, 1.0f };

        glm::mat4 Transform{ 1.0f };

        glm::mat4& GetTransform()
        {
            if (m_Dirty)
            {
                RecalculateTransform();
            }
            return Transform;
        }

        const glm::mat4& GetTransform() const
        {
            if (m_Dirty)
            {
                const_cast<TransformComponent*>(this)->RecalculateTransform();
            }
            return Transform;
        }

        operator glm::mat4() const
        {
            return GetTransform();
        }

    private:
        void RecalculateTransform()
        {
            glm::mat4 transform = glm::translate(glm::mat4(1.0f), Position);
            transform = glm::rotate(transform, glm::radians(Rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
            transform = glm::rotate(transform, glm::radians(Rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
            transform = glm::rotate(transform, glm::radians(Rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
            transform = glm::scale(transform, Scale);
            Transform = transform;
            m_Dirty = false;
        }

    public:
        void SetPosition(const glm::vec3& position) { Position = position; m_Dirty = true; }
        void SetDeltaPosition(const glm::vec3& delta) { Position += delta; m_Dirty = true; }
        void SetRotation(const glm::vec3& rotation) { Rotation = rotation; m_Dirty = true; }
        void SetDeltaRotation(const glm::vec3& delta) { Rotation += delta; m_Dirty = true; }
        void SetScale(const glm::vec3& scale) { Scale = scale; m_Dirty = true; }
        void SetDeltaScale(const glm::vec3& delta) { Scale += delta; m_Dirty = true; }

    private:
        bool m_Dirty = true;
    };

    struct SpriteRendererComponent
    {
        glm::vec4 Color{ 1.0f, 1.0f, 1.0f, 1.0f };
        Ref<Texture2D> Texture{ nullptr };
        float TilingFactor{ 1.0f };
    };

    struct CameraComponent
    {
        SceneCamera Camera;
        bool Primary = true;
        bool FixedAspectRatio = false;
    };

struct ScriptComponent
    {
        std::string ClassName;
        ScriptableEntity* Instance = nullptr;

        ScriptComponent() = default;
        ScriptComponent(const ScriptComponent&) = default;

        template<typename T>
        void BindComponent()
        {
            Instance = new T();
            Instance->m_Entity = Entity(entt::null, nullptr);
            ClassName = typeid(T).name();
        }
    };

}