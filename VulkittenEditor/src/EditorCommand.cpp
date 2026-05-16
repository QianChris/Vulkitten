#include "EditorCommand.h"

#include "Vulkitten/Scene/Scene.h"

namespace Vulkitten {

void CommandSystem::Execute(Ref<EditorCommand> command) {
    if (m_CurrentIndex < m_History.size()) {
        m_History.erase(m_History.begin() + m_CurrentIndex, m_History.end());
    }
    command->Execute();
    m_History.push_back(command);
    if (m_History.size() > MAX_HISTORY) {
        m_History.erase(m_History.begin());
    } else {
        m_CurrentIndex++;
    }
}

void CommandSystem::Undo() {
    if (m_CurrentIndex > 0) {
        m_CurrentIndex--;
        m_History[m_CurrentIndex]->Undo();
    }
}

void CommandSystem::Redo() {
    if (m_CurrentIndex < m_History.size()) {
        m_History[m_CurrentIndex]->Execute();
        m_CurrentIndex++;
    }
}

void CommandSystem::Clear() {
    m_History.clear();
    m_CurrentIndex = 0;
}

// ===== DestroyEntityCommand =====
DestroyEntityCommand::DestroyEntityCommand(Ref<Scene> scene, Entity entity)
    : m_Scene(scene), m_EntityID(entity.GetEntityID()) {
    if (entity.HasComponent<TagComponent>())
        m_Tag = entity.GetComponent<TagComponent>().Tag;
}

void DestroyEntityCommand::Execute() {
    if (!m_Scene) return;
    Entity entity((entt::entity)m_EntityID, m_Scene.get());
    if (entity) m_Scene->DestroyEntity(entity);
}

void DestroyEntityCommand::Undo() {
    // TODO: Entity restoration requires full component serialization.
    // Undo for destroy is not yet implemented without an entity archive.
}

// ===== SetTransformCommand =====
SetTransformCommand::SetTransformCommand(Entity entity, const TransformComponent& oldT, const TransformComponent& newT)
    : m_Entity(entity), m_OldTransform(oldT), m_NewTransform(newT) {}

void SetTransformCommand::Execute() {
    if (m_Entity && m_Entity.HasComponent<TransformComponent>())
        m_Entity.GetComponent<TransformComponent>() = m_NewTransform;
}

void SetTransformCommand::Undo() {
    if (m_Entity && m_Entity.HasComponent<TransformComponent>())
        m_Entity.GetComponent<TransformComponent>() = m_OldTransform;
}

} // namespace Vulkitten