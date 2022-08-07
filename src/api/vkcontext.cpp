#include "vkcontext.hpp"
#include "vkutils.hpp"
#include <stdexcept>

VulkanContext::VulkanContext() {}

VulkanContext::VulkanContext(GLFWwindow* window) {
    CreateInstance();
    PickPhysicalDevice();
}

void VulkanContext::Destroy() {
    if(useValidationLayers) {
        VkUtils::DestroyDebugMessenger(instance, debugMessenger, nullptr);
    }
    vkDestroyDevice(device, nullptr);
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

// Setting up debugging
    VkDebugUtilsMessengerCreateInfoEXT dMessenger {};
    PopulateDebugMessenger(dMessenger);
    instanceInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*) &dMessenger;
//

    auto extensions = VkUtils::GetExtensions();

    instanceInfo.enabledExtensionCount = extensions.size();
    instanceInfo.ppEnabledExtensionNames = extensions.data();

    auto layers = VkUtils::GetLayers();

    instanceInfo.enabledLayerCount = layers.size();
    instanceInfo.ppEnabledLayerNames = layers.data();

    if(vkCreateInstance(&instanceInfo, nullptr, &instance) != VK_SUCCESS) {
        throw std::runtime_error("Error when creating Instance");
    }
}

void VulkanContext::CreateDebugMessenger() {
    VkDebugUtilsMessengerCreateInfoEXT info;
    PopulateDebugMessenger(info);

    if(VkUtils::CreateDebugMessenger(instance, &info, nullptr, &debugMessenger) != VK_SUCCESS) {
        throw std::runtime_error("Error when creating VulkanContext's Debug Messenger");
    }
}

void VulkanContext::PopulateDebugMessenger(VkDebugUtilsMessengerCreateInfoEXT& debugMessenger) {
    debugMessenger.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    debugMessenger.pfnUserCallback = VkUtils::DebugCallback;
    debugMessenger.messageSeverity =
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT;
    debugMessenger.messageType = 
        VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
    debugMessenger.pUserData = nullptr;
}

void VulkanContext::PickPhysicalDevice() {
    physicalDevice = VK_NULL_HANDLE;
    uint32_t physicalDeviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, nullptr);

    if(physicalDeviceCount == 0) {
        throw std::runtime_error("No available Physical Devices with Vulkan support");
    }

    std::vector<VkPhysicalDevice> physicalDevices(physicalDeviceCount);
    vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, physicalDevices.data());

    for(auto device : physicalDevices) {
        if(VkUtils::IsDeviceSuitable(device)) {
            physicalDevice = device;
            break;
        }
    }

    if(physicalDevice == VK_NULL_HANDLE) {
        throw std::runtime_error("No suitable Physical Devices found");
    }
}

void VulkanContext::CreateLogicalDevice() {
    auto queueFamilies = VkUtils::FindQueueFamilies(physicalDevice);

    VkDeviceQueueCreateInfo queueInfo{};
    queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueInfo.queueCount = 1;
    queueInfo.queueFamilyIndex = queueFamilies.graphics;
    float priority = 1.0;
    queueInfo.pQueuePriorities = &priority;
// Here we specify features we want to have
// They are all initialized to VK_FALSE like this
    VkPhysicalDeviceFeatures features{};

// Device
    VkDeviceCreateInfo deviceInfo{};
    deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

    deviceInfo.queueCreateInfoCount = 1;
    deviceInfo.pQueueCreateInfos = &queueInfo;
    deviceInfo.pEnabledFeatures  = &features;

    deviceInfo.enabledExtensionCount = 0; // We'll come back for these

// In modern Vulkan, Device layers are ignored, as there's
// no longer a distinction between Device and Instance layers
// It is still, however, a good idea to set them as it allows for
// backwards compatibility
    if(useValidationLayers) {
        auto layers = VkUtils::GetLayers();
        deviceInfo.enabledLayerCount = layers.size();
        deviceInfo.ppEnabledLayerNames = layers.data();
    } else {
        deviceInfo.enabledLayerCount = 0;
    }

    if(vkCreateDevice(physicalDevice, &deviceInfo, nullptr, &device) != VK_SUCCESS) {
        throw std::runtime_error("Error when creating Logical Device");
    }

    vkGetDeviceQueue(
        device, 
        queueFamilies.graphics, 
        0 /* because we're only using one queue for this family */, 
        &graphicsQueue
    );
}