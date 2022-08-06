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

void VkUtils::CheckLayers() {
    ListLayers();
    uint32_t layerCount = 0;

    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
    std::vector<VkLayerProperties> layerProperties(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, layerProperties.data());

    for(auto requestedLayer : VALIDATION_LAYERS) {
        bool found = false;

        for(auto availableLayer : layerProperties) {
            if(strcmp(requestedLayer, availableLayer.layerName) == 0) {
                found = true;
                break;
            }
        }

        if(!found) { throw std::runtime_error("Requested Validation Layers could not be found"); }
    }
}

bool VkUtils::IsDeviceSuitable(VkPhysicalDevice device) {
    return true;
}

// VKAPI_ATTR VkBool32 VKAPI_CALL VkUtils::DebugCallback(
//     VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
//     VkDebugUtilsMessageTypeFlagBitsEXT messageType,
//     const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
//     void* pUserData
// ) {

//     const auto WARNING = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;

//     if(messageSeverity > WARNING) {
//         std::cout << "[ERROR] " << pCallbackData->pMessage << "\n";
//     }

//     return VK_FALSE;
// }

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