#pragma once
#include "IPanel.h"

namespace Vulkitten {
    class PerformancePanel : public IPanel {
    public:
        PerformancePanel() = default;
        void OnAttach(EditorContext* context) override;
        void OnUIRender() override;
    };
} // namespace Vulkitten