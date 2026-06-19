#include "vktpch.h"
#include "Device.h"

#include "Vulkitten/Core/ClassFactory.h"

namespace Vulkitten {

Device& Device::Get()
{
    return ClassFactory::GetInterface<Device>();
}

} // namespace Vulkitten
