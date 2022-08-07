#include "vkutils.hpp"
#include "vkcontext.hpp"

#include <stdexcept>
#include <cstring>
#include <iostream>

std::vector<const char*> VkUtils::GetExtensions() {
    uint32_t extensionCount = 0;
    auto extensions = glfwGetRequiredInstanceExtensions(&extensionCount);
    std::vector<const char*> exts(extensions, extensions + extensionCount);

    if(useValidationLayers) {
        exts.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    return exts;
}

void ListLayers() {
    uint32_t layerCount = 0;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
    std::vector<VkLayerProperties> layerProperties(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, layerProperties.data());

    for(auto availableLayer : layerProperties) {
        std::cout << "\t- Available Layer (" << availableLayer.layerName << ")\n";
    }
}

std::vector<const char*> VkUtils::GetLayers() {
    ListLayers();
    uint32_t layerCount = 0;

    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
    std::vector<VkLayerProperties> layerProperties(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, layerProperties.data());

    for(auto requestedLayer : VALIDATION_LAYERS) {
        for(auto availableLayer : layerProperties) {
            if(strcmp(requestedLayer, availableLayer.layerName) == 0) {
                return { requestedLayer };
            }
        }
    }
    
    throw std::runtime_error("No Validation Layers found");
}

bool VkUtils::IsDeviceSuitable(VkPhysicalDevice device) {
    // Properties are basic things like name, device type and API version
    VkPhysicalDeviceProperties properties;
    // Features are additional stuff like VR support, geometry shaders and so on
    VkPhysicalDeviceFeatures features;

    vkGetPhysicalDeviceProperties(device, &properties);
    vkGetPhysicalDeviceFeatures(device, &features);
    
    std::cout << "Available GPU: " << properties.deviceName << "\n";

    // Get queue families
    auto queueFamilies = VkUtils::FindQueueFamilies(device);

    // We want a discrete GPU with geometry shader support, we could make this
    // more complex if we wanted, like a ranking system between available devices
    return properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU &&
        features.geometryShader == VK_TRUE &&
        queueFamilies.IsComplete();
}

QueueFamilyIndices VkUtils::FindQueueFamilies(VkPhysicalDevice device) {
    QueueFamilyIndices indices;

    uint32_t queueFamilyCount;

    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    int i = 0;
    for(auto queueFamily : queueFamilies) {
        if(queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            indices.graphics = i;
        }
        if(indices.IsComplete()) break;
        i++;
    }

    return indices;
}


VKAPI_ATTR VkBool32 VKAPI_CALL VkUtils::DebugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData) 
{
    const auto WARNING = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;

    if(messageSeverity > WARNING) {
        std::cout << "[ERROR] " << pCallbackData->pMessage << "\n";
    }

    return VK_FALSE;
}

VkResult VkUtils::CreateDebugMessenger(
    VkInstance instance,
    const VkDebugUtilsMessengerCreateInfoEXT* info,
    const VkAllocationCallbacks* alloc,
    VkDebugUtilsMessengerEXT* messenger
) {
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");

    if(func != nullptr) {
        return func(instance, info, alloc, messenger);
    }
    return VK_ERROR_EXTENSION_NOT_PRESENT;
}

VkResult VkUtils::DestroyDebugMessenger(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) {
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(instance, debugMessenger, pAllocator);
    }
}