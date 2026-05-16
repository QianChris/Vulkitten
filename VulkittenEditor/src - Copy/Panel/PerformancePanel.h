#pragma once

#include "Vulkitten/Core/Core.h"
#include "Vulkitten/Core/Application.h"
#include "Vulkitten/Scene/Scene.h"

namespace Vulkitten {

    class PerformancePanel
    {
    public:
        PerformancePanel() = default;
        PerformancePanel(const Ref<Scene>& scene);

        void SetContext(const Ref<Scene>& scene);
        void OnImGuiRender();

    private:
        Ref<Scene> m_Context;
    };

}