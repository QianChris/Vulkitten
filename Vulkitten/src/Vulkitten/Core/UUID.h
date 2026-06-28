#pragma once

#include "Core.h"

namespace Vulkitten {

    class VKT_API UUID
    {
    public:
        UUID();
        UUID(uint64_t uuid);
        UUID(const UUID& other) = default;

        operator uint64_t() const { return m_UUID; }

    private:
        uint64_t m_UUID;
    };

}

namespace std {
	template <typename T> struct hash;

	template<>
	struct hash<Vulkitten::UUID>
	{
		std::size_t operator()(const Vulkitten::UUID& uuid) const
		{
			return (uint64_t)uuid;
		}
	};

}