#include "vktpch.h"
#include "VulkanInstance.h"

#ifdef VKT_HAS_VULKAN
#include <vulkan/vulkan.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#endif

#include <vector>
#include "Vulkitten/Perf/Instrumentor.h"

namespace Vulkitten {

VulkanInstance::VulkanInstance()
{
}

VulkanInstance::~VulkanInstance()
{
    Shutdown();
}

void VulkanInstance::Init(bool enableValidationLayers)
{
    VKT_PROFILE_FUNCTION();

#ifdef VKT_HAS_VULKAN
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Vulkitten";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "Vulkitten";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_3;

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    // Validation layers
    const char* validationLayer = "VK_LAYER_KHRONOS_validation";
    if (enableValidationLayers)
    {
        createInfo.enabledLayerCount = 1;
        createInfo.ppEnabledLayerNames = &validationLayer;
    }

    // Required extensions (always include surface support)
    std::vector<const char*> extensions;
    extensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
#ifdef VK_USE_PLATFORM_WIN32_KHR
    extensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
#endif
    if (enableValidationLayers)
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();

    VkInstance instance;
    VkResult result = vkCreateInstance(&createInfo, nullptr, &instance);
    if (result == VK_SUCCESS)
    {
        m_Instance = instance;
        VKT_CORE_INFO("VulkanInstance: VkInstance created successfully");
    }
    else
    {
        VKT_CORE_WARN("VulkanInstance: vkCreateInstance failed with result {0}", static_cast<int>(result));
    }
#else
    (void)enableValidationLayers;
    VKT_CORE_INFO("VulkanInstance: Vulkan SDK not available (VKT_HAS_VULKAN not defined)");
#endif
}

void VulkanInstance::Shutdown()
{
#ifdef VKT_HAS_VULKAN
    if (m_Instance)
    {
        vkDestroyInstance(static_cast<VkInstance>(m_Instance), nullptr);
        m_Instance = nullptr;
    }
    m_DebugMessenger = nullptr;
#endif
}

} // namespace Vulkitten
