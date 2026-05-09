#include "vktpch.h"
#include "SceneSerializer.h"

#include "Scene.h"
#include "Entity.h"
#include "Components.h"

#include <yaml-cpp/yaml.h>
#include "Vulkitten/Utils/YAMLConversions.h"

namespace Vulkitten {

    SceneSerializer::SceneSerializer(const Ref<Scene>& scene)
        : m_Scene(scene)
    {
    }

    void SceneSerializer::Serialize(const std::string& filepath)
    {
        YAML::Emitter out;
        out << YAML::BeginMap;
        out << YAML::Key << "Scene" << YAML::Value << "Untitled";
        out << YAML::Key << "Entities" << YAML::Value << YAML::BeginSeq;
        for (auto entity : m_Scene->GetRegistry().view<entt::entity>())
        {
            Entity ent = {entity, m_Scene.get()};
            if (!ent)
                continue;
            SerializeEntity(out, ent);
        }
        out << YAML::EndSeq;
        out << YAML::EndMap;

        std::ofstream fout(filepath);
        fout << out.c_str();
    }

    bool SceneSerializer::Deserialize(const std::string& filepath)
    {
        std::ifstream stream(filepath);
        if (!stream.is_open())
            return false;

        try
        {
            YAML::Node data = YAML::Load(stream);
            if (!data["Scene"])
                return false;

            std::string sceneName = data["Scene"].as<std::string>();
            VKT_CORE_INFO("Deserializing scene '{0}'", sceneName);

            if (data["Entities"])
            {
                auto entities = data["Entities"];
                if (entities.IsSequence())
                {
                    for (auto entity : entities)
                    {
                        DeserializeEntity(entity);
                    }
                }
            }
        }
        catch (const YAML::ParserException& e)
        {
            VKT_CORE_ERROR("Failed to parse scene file: {0}", e.what());
            return false;
        }

return true;
    }

    void SceneSerializer::SerializeEntity(YAML::Emitter& out, Entity entity)
    {
        out << YAML::BeginMap;
        out << YAML::Key << "EntityID" << YAML::Value << entity.GetEntityID();
        if (entity.HasComponent<TagComponent>())
        {
            auto& tag = entity.GetComponent<TagComponent>().Tag;
            out << YAML::Key << "TagComponent" << YAML::Value << tag;
        }
        if (entity.HasComponent<TransformComponent>())
        {
            auto& transform = entity.GetComponent<TransformComponent>();
            out << YAML::Key << "TransformComponent" << YAML::Value << YAML::BeginMap;
            out << YAML::Key << "Position" << YAML::Value << YAML::Encode(transform.Position);
            out << YAML::Key << "Rotation" << YAML::Value << YAML::Encode(transform.Rotation);
            out << YAML::Key << "Scale" << YAML::Value << YAML::Encode(transform.Scale);
            out << YAML::EndMap;
        }
        if (entity.HasComponent<CameraComponent>())
        {
            auto& camera = entity.GetComponent<CameraComponent>();
            out << YAML::Key << "CameraComponent" << YAML::Value << YAML::BeginMap;
            out << YAML::Key << "Primary" << YAML::Value << camera.Primary;
            out << YAML::Key << "FixedAspectRatio" << YAML::Value << camera.FixedAspectRatio;
            out << YAML::Key << "Camera" << YAML::Value << YAML::BeginMap;
            out << YAML::Key << "ProjectionType" << YAML::Value << static_cast<int>(camera.Camera.GetProjectionType());
            out << YAML::Key << "PerspectiveFOV" << YAML::Value << camera.Camera.GetPerspectiveFOV();
            out << YAML::Key << "PerspectiveNear" << YAML::Value << camera.Camera.GetPerspectiveNear();
            out << YAML::Key << "PerspectiveFar" << YAML::Value << camera.Camera.GetPerspectiveFar();
            out << YAML::Key << "OrthographicSize" << YAML::Value << camera.Camera.GetOrthographicSize();
            out << YAML::Key << "OrthographicNear" << YAML::Value << camera.Camera.GetOrthographicNear();
            out << YAML::Key << "OrthographicFar" << YAML::Value << camera.Camera.GetOrthographicFar();
            out << YAML::EndMap;
            out << YAML::EndMap;
        }
        if (entity.HasComponent<SpriteRendererComponent>())
        {
            auto& sprite = entity.GetComponent<SpriteRendererComponent>();
            out << YAML::Key << "SpriteRendererComponent" << YAML::Value << YAML::BeginMap;
            out << YAML::Key << "Color" << YAML::Value << YAML::Encode(sprite.Color);
            out << YAML::Key << "TilingFactor" << YAML::Value << sprite.TilingFactor;
            out << YAML::EndMap;
        }
        out << YAML::EndMap;
    }

    void SceneSerializer::DeserializeEntity(const YAML::Node& entityNode)
    {
        std::string name;
        if (entityNode["TagComponent"])
        {
            name = entityNode["TagComponent"].as<std::string>();
        }

        Entity entity = m_Scene->CreateEntity(name);

        if (entityNode["TransformComponent"])
        {
            auto transformNode = entityNode["TransformComponent"];
            auto& transform = entity.GetComponent<TransformComponent>();
            transform.Position = transformNode["Position"].as<glm::vec3>();
            transform.Rotation = transformNode["Rotation"].as<glm::vec3>();
            transform.Scale = transformNode["Scale"].as<glm::vec3>();
        }

        if (entityNode["CameraComponent"])
        {
            auto cameraNode = entityNode["CameraComponent"];
            auto& cameraComp = entity.AddComponent<CameraComponent>();
            cameraComp.Primary = cameraNode["Primary"].as<bool>();
            cameraComp.FixedAspectRatio = cameraNode["FixedAspectRatio"].as<bool>();

            if (cameraNode["Camera"])
            {
                auto cameraData = cameraNode["Camera"];
                if (cameraData["ProjectionType"])
                {
                    cameraComp.Camera.SetProjectionType(
                        cameraData["ProjectionType"].as<int>() == 0
                        ? SceneCamera::ProjectionType::Perspective
                        : SceneCamera::ProjectionType::Orthographic);
                }
                if (cameraData["PerspectiveFOV"])
                    cameraComp.Camera.SetPerspectiveFOV(cameraData["PerspectiveFOV"].as<float>());
                if (cameraData["PerspectiveNear"])
                    cameraComp.Camera.SetPerspectiveNear(cameraData["PerspectiveNear"].as<float>());
                if (cameraData["PerspectiveFar"])
                    cameraComp.Camera.SetPerspectiveFar(cameraData["PerspectiveFar"].as<float>());
                if (cameraData["OrthographicSize"])
                    cameraComp.Camera.SetOrthographicSize(cameraData["OrthographicSize"].as<float>());
                if (cameraData["OrthographicNear"])
                    cameraComp.Camera.SetOrthographicNear(cameraData["OrthographicNear"].as<float>());
                if (cameraData["OrthographicFar"])
                    cameraComp.Camera.SetOrthographicFar(cameraData["OrthographicFar"].as<float>());
            }
        }

        if (entityNode["SpriteRendererComponent"])
        {
            auto spriteNode = entityNode["SpriteRendererComponent"];
            auto& sprite = entity.AddComponent<SpriteRendererComponent>();
            sprite.Color = spriteNode["Color"].as<glm::vec4>();
            sprite.TilingFactor = spriteNode["TilingFactor"].as<float>();
        }
    }

}