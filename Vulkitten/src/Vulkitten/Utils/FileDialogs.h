#pragma once

#include "Vulkitten/Core/Core.h"

namespace Vulkitten {

    class VKT_API FileDialogs
    {
    public:
        std::string OpenFile(const char* filter);
        std::string SaveFile(const char* filter);
    };

}