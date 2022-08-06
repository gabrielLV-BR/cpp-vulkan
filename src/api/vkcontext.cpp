#include "vkcontext.hpp"
#include "vkutils.hpp"
#include <stdexcept>

VulkanContext::VulkanContext() {}

VulkanContext::VulkanContext(GLFWwindow* window) {
    CreateInstance();
}

void VulkanContext::Destroy() {
    vkDestroyInstance(instance, nullptr);
}

void VulkanContext::CreateInstance() {
    VkApplicationInfo appInfo {};
    appInfo.pApplicationName = "My Vulkan App";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);

    appInfo.pEngineName = "My Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);

    appInfo.apiVersion = VK_API_VERSION_1_1;
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;

    VkInstanceCreateInfo instanceInfo {};
    instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceInfo.pApplicationInfo = &appInfo;

    auto extensions = VkUtils::GetExtensions();

    instanceInfo.enabledExtensionCount = extensions.size();
    instanceInfo.ppEnabledExtensionNames = extensions.data();

    VkUtils::CheckLayers();

    instanceInfo.enabledLayerCount = VALIDATION_LAYERS.size();
    instanceInfo.ppEnabledLayerNames = VALIDATION_LAYERS.data();

    if(vkCreateInstance(&instanceInfo, nullptr, &instance) != VK_SUCCESS) {
        throw std::runtime_error("Error when creating Instance");
    }
}