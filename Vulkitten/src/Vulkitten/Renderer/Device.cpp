#include "vktpch.h"
#include "Device.h"

#include "Vulkitten/Core/ClassFactory.h"

namespace Vulkitten {

IDevice& IDevice::Get()
{
    return ClassFactory::GetInterface<IDevice>();
}

} // namespace Vulkitten
