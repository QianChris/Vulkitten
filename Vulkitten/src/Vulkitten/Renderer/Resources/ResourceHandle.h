#pragma once

#include "Vulkitten/Core/Core.h"

namespace Vulkitten {

    template<typename T>
    struct Handle {
        uint32_t index = UINT32_MAX;
        uint16_t generation = 0;
        
        bool IsValid() const { return index != UINT32_MAX; }
        bool operator==(const Handle& o) const { return index == o.index && generation == o.generation; }
        bool operator!=(const Handle& o) const { return !(*this == o); }
        bool operator<(const Handle& o) const { return index < o.index; }  // 用于排序
    };

} // namespace Vulkitten