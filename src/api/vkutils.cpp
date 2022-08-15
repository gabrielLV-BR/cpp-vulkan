#include "vkutils.hpp"
#include "vkcontext.hpp"

#include "utils/debug.hpp"

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

void VkUtils::ListLayers() {
    uint32_t layerCount = 0;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
    std::vector<VkLayerProperties> layerProperties(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, layerProperties.data());

    for(auto& availableLayer : layerProperties) {
        std::cout << "\t- Available Layer (" << availableLayer.layerName << ")\n";
    }
}

std::vector<const char*> VkUtils::GetLayers() {
    uint32_t layerCount = 0;

    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
    std::vector<VkLayerProperties> layerProperties(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, layerProperties.data());

    for(auto& requestedLayer : VALIDATION_LAYERS) {
        for(auto& availableLayer : layerProperties) {
            if(strcmp(requestedLayer, availableLayer.layerName) == 0) {
                return { requestedLayer };
            }
        }
    }
    
    throw std::runtime_error("No Validation Layers found");
}

bool DeviceSupportsExtensions(VkPhysicalDevice device) {
    uint32_t extensionCount;

    vkEnumerateDeviceExtensionProperties(
        device, nullptr, &extensionCount, nullptr
    );

    std::vector<VkExtensionProperties> 
        availableExtensions(extensionCount);

    VK_ASSERT(
        vkEnumerateDeviceExtensionProperties(
            device, 
            nullptr, 
            &extensionCount, 
            availableExtensions.data()
        )
    );

    for(auto& requiredExtension : DEVICE_EXTENSIONS) {
        bool found = false;
        for(auto& availableExtension : availableExtensions) {
            if(strcmp(requiredExtension, availableExtension.extensionName) == 0) {
                printf("\t- Found extension %s\n", requiredExtension);
                found = true;
                break;
            }
        }
        if(!found) return false;
    }

    return true;
}

bool VkUtils::IsDeviceSuitable(
    VkPhysicalDevice device,
    VkSurfaceKHR surface
) {
    // Properties are basic things like name, device type and API version
    VkPhysicalDeviceProperties properties;
    // Features are additional stuff like VR support, geometry shaders and so on
    VkPhysicalDeviceFeatures features;

    vkGetPhysicalDeviceProperties(device, &properties);
    vkGetPhysicalDeviceFeatures(device, &features);
    
    // Get queue families
    auto queueFamilies = VkUtils::FindQueueFamilies(device, surface);
    bool extensionsSupported = DeviceSupportsExtensions(device);

    bool swapchainAdequate = false;
    if(extensionsSupported) {
        auto swapchainDetails = 
            VkUtils::FindSwapchainSupport(device, surface);

        swapchainAdequate = 
            !swapchainDetails.formats.empty() &&
            !swapchainDetails.presentModes.empty();
    }

#ifndef NDEBUG
    std::cout << "Available GPU: " << properties.deviceName << "\n";
    std::cout << "Device " << 
        (extensionsSupported ? "" : "DOES NOT ")
    << "support extensions\n";
#endif

    // We want a discrete GPU with geometry shader support, we could make this
    // more complex if we wanted, like a ranking system between available devices
    return 
        extensionsSupported &&
        swapchainAdequate   &&
        queueFamilies.IsComplete();
}

int times = 1, reqs = 1;
QueueFamilyIndices VkUtils::FindQueueFamilies(
    VkPhysicalDevice device,
    VkSurfaceKHR surface
) {
    static optional<QueueFamilyIndices> indices;

#ifndef NDEBUG
    std::cout << "Requested for queue families " << reqs++ << " times\n";
#endif

    // Prevents recalculations
    if(indices.has_value()) return indices.value();

#ifndef NDEBUG
    std::cout << "Searched for queue families " << times++ << " times\n";
#endif

    QueueFamilyIndices _indices;
    uint32_t queueFamilyCount;

    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    int i = 0;
    for(auto& queueFamily : queueFamilies) {
        if(queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            _indices.graphics = i;
        }

        VkBool32 presentSupport = VK_FALSE;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
        if(presentSupport == VK_TRUE) {
            _indices.present = i;
        }

        if(_indices.IsComplete()) break;
        i++;
    }

    indices.emplace(_indices);

    return indices.value();
}

SwapchainSupport VkUtils::FindSwapchainSupport(
    VkPhysicalDevice device,
    VkSurfaceKHR surface
) {
    static optional<SwapchainSupport> _support;

    if(_support.has_value()) return _support.value();

    SwapchainSupport support{};

    { // Surface capabilities
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
            device, surface, &support.capabilites
        );
    }

    { // Surface Formats
        uint32_t formatCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR(
            device, surface, &formatCount, nullptr
        );

        if(formatCount != 0) {
            support.formats.resize(formatCount);
            vkGetPhysicalDeviceSurfaceFormatsKHR(
                device, surface, &formatCount, support.formats.data()
            );
        }
    }

    { // Present Modes
        uint32_t modeCount;
        vkGetPhysicalDeviceSurfacePresentModesKHR(
            device, surface, &modeCount, nullptr
        );

        if(modeCount != 0) {
            support.presentModes.resize(modeCount);
            vkGetPhysicalDeviceSurfacePresentModesKHR(
                device, surface, &modeCount, support.presentModes.data()
            );
        }
    }

    _support.emplace(support);

    return support;
}

void VkUtils::RecordCmdBuffer(VkCommandBuffer&, uint32_t imageIndex) {
    
}


// DEBUG

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
        return VK_SUCCESS;
    }
    return VK_ERROR_UNKNOWN;
}