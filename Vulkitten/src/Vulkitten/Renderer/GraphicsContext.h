#pragma once

#include "Vulkitten/Core/Core.h"

namespace Vulkitten {

    class GraphicsContext
    {
    public:
        virtual ~GraphicsContext() = default;

        virtual void Init() = 0;
        virtual void SwapBuffers() = 0;
    };

}