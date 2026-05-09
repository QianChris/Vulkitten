#pragma once

#include "Vulkitten/Core/Core.h"

#include <yaml-cpp/yaml.h>

namespace Vulkitten {

    class Scene;
    class Entity;

    class VKT_API SceneSerializer
    {
    public:
        SceneSerializer(const Ref<Scene>& scene);

        void Serialize(const std::string& filepath);
        bool Deserialize(const std::string& filepath);
        
    private:
        void SerializeEntity(YAML::Emitter& out, Entity entity);
        void DeserializeEntity(const YAML::Node& entityNode);

    private:
        Ref<Scene> m_Scene;
    };

}