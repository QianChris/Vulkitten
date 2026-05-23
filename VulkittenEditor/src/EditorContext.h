#pragma once
#include "Vulkitten/Core/Core.h"
#include "Vulkitten/Scene/Scene.h"
#include "Vulkitten/Scene/Entity.h"
#include "Vulkitten/Renderer/EditorCamera.h"
#include <functional>
#include <typeindex>
#include <unordered_map>
#include <vector>

namespace Vulkitten {

    // ===== 强类型信号定义 =====
    namespace EditorEvents {
        struct EntitySelected {
            Entity entity;
            EntitySelected() = default;
            explicit EntitySelected(Entity e) : entity(e) {}
        };

        struct EntityHovered {
            Entity entity;
            EntityHovered() = default;
            explicit EntityHovered(Entity e) : entity(e) {}
        };

        struct EntityDestroyed {
            uint32_t entityID;
            EntityDestroyed() = default;
            explicit EntityDestroyed(uint32_t id) : entityID(id) {}
        };

        struct ComponentModified {
            Entity entity;
            const char* componentName;
            ComponentModified() = default;
            ComponentModified(Entity e, const char* name) : entity(e), componentName(name) {}
        };

        struct SceneModified {
            SceneModified() = default;
        };

        struct ViewportResized {
            uint32_t width;
            uint32_t height;
            ViewportResized() = default;
            ViewportResized(uint32_t w, uint32_t h) : width(w), height(h) {}
        };

        struct RequestNewScene {
            RequestNewScene() = default;
        };

        struct RequestOpenScene {
            RequestOpenScene() = default;
        };

        struct RequestSaveScene {
            RequestSaveScene() = default;
        };
    }

    // ===== 信号总线（非全局，支持多实例） =====
    class SignalBus {
    public:
        template<typename SignalT>
        using Handler = std::function<void(const SignalT&)>;

        template<typename SignalT>
        void Subscribe(Handler<SignalT> handler) {
            HandlerWrapper wrapper;
            wrapper.callback = [handler](const void* ptr) {
                handler(*static_cast<const SignalT*>(ptr));
                };
            m_Handlers[typeid(SignalT)].push_back(std::move(wrapper));
        }

        template<typename SignalT, typename... Args>
        void Publish(Args&&... args) {
            SignalT signal(std::forward<Args>(args)...);
            Publish(signal);
        }

        template<typename SignalT>
        void Publish(const SignalT& signal) {
            auto it = m_Handlers.find(typeid(SignalT));
            if (it != m_Handlers.end()) {
                for (auto& h : it->second)
                    h.callback(&signal);
            }
        }

    private:
        struct HandlerWrapper {
            std::function<void(const void*)> callback;
        };
        std::unordered_map<std::type_index, std::vector<HandlerWrapper>> m_Handlers;
    };

    // ===== 上下文（单一事实来源） =====
    class CommandSystem; // 前置声明

    struct EditorContext {
        Ref<Scene> scene;
        Entity selectedEntity;
        Entity hoveredEntity;
        EditorCamera* editorCamera = nullptr;
        SignalBus signals;
        CommandSystem* commands = nullptr;   // 由 EditorLayer 注入

        enum class EditorState {
            Edit = 0,
            Play = 1,
            Simulate = 2
		};
		EditorState state = EditorState::Edit;
		bool isEditorCameraActive = true;

        void SelectEntity(Entity entity) {
            selectedEntity = entity;
            signals.Publish<EditorEvents::EntitySelected>(entity);
        }
        void SetHoveredEntity(Entity entity) {
            hoveredEntity = entity;
            signals.Publish<EditorEvents::EntityHovered>(entity);
        }
    };

} // namespace Vulkitten