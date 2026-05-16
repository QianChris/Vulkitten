#pragma once
#include "Vulkitten/Core/Core.h"
#include "Vulkitten/Scene/Entity.h"
#include "Vulkitten/Scene/Components.h"
#include <glm/glm.hpp>
#include <vector>
#include <string>

namespace Vulkitten {

class EditorCommand {
public:
    virtual ~EditorCommand() = default;
    virtual void Execute() = 0;
    virtual void Undo() = 0;
    virtual const char* GetName() const = 0;
};

class CommandSystem {
public:
    void Execute(Ref<EditorCommand> command);
    void Undo();
    void Redo();
    bool CanUndo() const { return m_CurrentIndex > 0; }
    bool CanRedo() const { return m_CurrentIndex < m_History.size(); }
    void Clear();

private:
    std::vector<Ref<EditorCommand>> m_History;
    size_t m_CurrentIndex = 0;
    static constexpr size_t MAX_HISTORY = 50;
};

class DestroyEntityCommand : public EditorCommand {
    Ref<Scene> m_Scene;
    uint32_t m_EntityID;
    std::string m_Tag;
public:
    DestroyEntityCommand(Ref<Scene> scene, Entity entity);
    void Execute() override;
    void Undo() override;
    const char* GetName() const override { return "Destroy Entity"; }
};

class SetTransformCommand : public EditorCommand {
    Entity m_Entity;
    TransformComponent m_OldTransform;
    TransformComponent m_NewTransform;
public:
    SetTransformCommand(Entity entity, const TransformComponent& oldT, const TransformComponent& newT);
    void Execute() override;
    void Undo() override;
    const char* GetName() const override { return "Set Transform"; }
};

} // namespace Vulkitten