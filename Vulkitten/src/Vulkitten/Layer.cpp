#include "vktpch.h"
#include "Vulkitten/Layer.h"

namespace Vulkitten
{
    Layer::Layer(const std::string& name)
        : m_DebugName(name)
    {
    }

    Layer::~Layer()
    {
    }
}