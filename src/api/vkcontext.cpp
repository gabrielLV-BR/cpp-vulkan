#include "vkcontext.hpp"
#include "vkutils.hpp"

#include <stdexcept>
#include <iterator>
#include <set>

#include <limits>
#include <algorithm>
#include <cstdint>

VulkanContext::VulkanContext() {}

VulkanContext::VulkanContext(GLFWwindow *window)
{
    CreateInstance();
    CreateSurface(window);
    PickPhysicalDevice();
    CreateLogicalDevice();
    CreateSwapchain(window);
}

void VulkanContext::Destroy()
{
    if (useValidationLayers)
    {
        VkUtils::DestroyDebugMessenger(instance, debugMessenger, nullptr);
    }
    vkDestroySwapchainKHR(device, swapchain, nullptr);
    vkDestroyDevice(device, nullptr);
    vkDestroySurfaceKHR(instance, surface, nullptr);
    vkDestroyInstance(instance, nullptr);
}

void VulkanContext::CreateInstance()
{
    VkApplicationInfo appInfo{};
    appInfo.pApplicationName = "My Vulkan App";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);

    appInfo.pEngineName = "My Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);

    appInfo.apiVersion = VK_API_VERSION_1_1;
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;

    VkInstanceCreateInfo instanceInfo{};
    instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceInfo.pApplicationInfo = &appInfo;

    // Setting up debugging
    VkDebugUtilsMessengerCreateInfoEXT dMessenger{};
    PopulateDebugMessenger(dMessenger);
    instanceInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT *)&dMessenger;
    //

    auto extensions = VkUtils::GetExtensions();

    instanceInfo.enabledExtensionCount = extensions.size();
    instanceInfo.ppEnabledExtensionNames = extensions.data();

#ifndef NDEBUG
    VkUtils::ListLayers();
#endif

    activeLayers = VkUtils::GetLayers();

    instanceInfo.enabledLayerCount = activeLayers.size();
    instanceInfo.ppEnabledLayerNames = activeLayers.data();

    if (vkCreateInstance(&instanceInfo, nullptr, &instance) != VK_SUCCESS)
    {
        throw std::runtime_error("Error when creating Instance");
    }
}

void VulkanContext::CreateSurface(GLFWwindow *window)
{
    if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS)
    {
        throw std::runtime_error("Error when creating surface");
    }
}

void VulkanContext::CreateDebugMessenger()
{
    VkDebugUtilsMessengerCreateInfoEXT info;
    PopulateDebugMessenger(info);

    if (VkUtils::CreateDebugMessenger(instance, &info, nullptr, &debugMessenger) != VK_SUCCESS)
    {
        throw std::runtime_error("Error when creating VulkanContext's Debug Messenger");
    }
}

void VulkanContext::PopulateDebugMessenger(VkDebugUtilsMessengerCreateInfoEXT &debugMessenger)
{
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

void VulkanContext::PickPhysicalDevice()
{
    physicalDevice = VK_NULL_HANDLE;
    uint32_t physicalDeviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, nullptr);

    if (physicalDeviceCount == 0)
    {
        throw std::runtime_error("No available Physical Devices with Vulkan support");
    }

    std::vector<VkPhysicalDevice> physicalDevices(physicalDeviceCount);
    vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, physicalDevices.data());

    for (auto device : physicalDevices)
    {
        if (VkUtils::IsDeviceSuitable(device, surface))
        {
            physicalDevice = device;
            break;
        }
    }

    if (physicalDevice == VK_NULL_HANDLE)
    {
        throw std::runtime_error("No suitable Physical Devices found");
    }
}

void VulkanContext::CreateLogicalDevice()
{
    auto indices = VkUtils::FindQueueFamilies(physicalDevice, surface);

    // Since we have multiple queues, we must provide an
    // array of queue infos.
    // As shown in LearnVulkan.com, a `set` is a nice way
    // to make this more compact AND remove duplicate indices
    // (which is likely to happen with `graphics` and `present`)

    using std::vector, std::set;

    vector<VkDeviceQueueCreateInfo> queueCreateInfos;

    set<uint32_t> uniqueQueueFamilies = {
        indices.graphics.value(), indices.present.value()
    };
    
    float queuePriority = 1.0f;
    for (uint32_t queueFamily : uniqueQueueFamilies) {
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType =
            VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;

        queueCreateInfos.push_back(queueCreateInfo);
    }

    // They are all initialized to VK_FALSE like this
    VkPhysicalDeviceFeatures features{};

    // Device
    VkDeviceCreateInfo deviceInfo{};
    
    deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

    deviceInfo.queueCreateInfoCount = 
        static_cast<uint32_t>(queueCreateInfos.size());
    deviceInfo.pQueueCreateInfos = queueCreateInfos.data();
    deviceInfo.pEnabledFeatures = &features;

    deviceInfo.enabledExtensionCount = 
        static_cast<uint32_t>(DEVICE_EXTENSIONS.size()); // We'll come back for these
    deviceInfo.ppEnabledExtensionNames =
        DEVICE_EXTENSIONS.data();

    // In modern Vulkan, Device layers are ignored, as there's
    // no longer a distinction between Device and Instance layers
    // It is still, however, a good idea to set them as it allows for
    // backwards compatibility
    if (useValidationLayers) {
        deviceInfo.enabledLayerCount = activeLayers.size();
        deviceInfo.ppEnabledLayerNames = activeLayers.data();
    }
    else {
        deviceInfo.enabledLayerCount = 0;
    }

    if ( 
        vkCreateDevice(
            physicalDevice, 
            &deviceInfo, 
            nullptr, 
            &device
        ) != VK_SUCCESS ){
        throw std::runtime_error("Error when creating Logical Device");
    }

    vkGetDeviceQueue(
        device,
        indices.graphics.value(),
        0 /* because we're only using one queue for this family */,
        &graphicsQueue);

    vkGetDeviceQueue(
        device,
        indices.present.value(),
        0 /* because we're only using one queue for this family */,
        &presentQueue);
}

void VulkanContext::CreateSwapchain(GLFWwindow* window) {
    auto swapchainDetails = VkUtils::FindSwapchainSupport(physicalDevice, surface);

    VkSwapchainCreateInfoKHR swapchainInfo{};

    swapchainInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchainInfo.clipped = VK_TRUE;
    swapchainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchainInfo.surface = surface;

    // We need 3 informations to create the swapchain

    VkSurfaceFormatKHR surfaceFormat;
    { // Surface format
        // Default, in case we don't find any
        surfaceFormat = swapchainDetails.formats[0];
        // Search for the desired one
        for(auto& format : swapchainDetails.formats) {
            if(format.format == VK_FORMAT_R8G8B8A8_SRGB &&
            format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                surfaceFormat = format;
                break;
            }
        }
    }

    VkPresentModeKHR presentMode;
    { // Present mode
        // Default
        presentMode = swapchainDetails.presentModes[0];
        // Search for desired
        for(auto& mode : swapchainDetails.presentModes) {
            if (
                mode == VK_PRESENT_MODE_MAILBOX_KHR ||
                mode == VK_PRESENT_MODE_FIFO_KHR
            ) {
                presentMode = mode;
                break;
            }
        }
    }

    VkExtent2D extent;
    { // Extent2D
        auto& cap = swapchainDetails.capabilites;
        extent = cap.currentExtent;

        if(
            cap.currentExtent.width == 
            std::numeric_limits<uint32_t>::max()
        ) {
            int width, height;

            glfwGetFramebufferSize(window, &width, &height);

            extent = {
                std::clamp(
                    static_cast<uint32_t>(width),
                    cap.minImageExtent.width,
                    cap.maxImageExtent.width
                ),
                std::clamp(
                    static_cast<uint32_t>(height),
                    cap.minImageExtent.height,
                    cap.maxImageExtent.height
                )
            };
        }
    }

    swapchainInfo.imageExtent = extent;
    swapchainInfo.presentMode = presentMode;
    swapchainInfo.imageFormat = surfaceFormat.format;
    swapchainInfo.imageColorSpace = surfaceFormat.colorSpace;

    auto imageCount = std::clamp(
        swapchainDetails.capabilites.minImageCount + 1,
        swapchainDetails.capabilites.minImageCount,
        swapchainDetails.capabilites.maxImageCount
    );

    swapchainInfo.minImageCount = imageCount;
    swapchainInfo.imageArrayLayers = 1;
    swapchainInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    auto queueIndices = VkUtils::FindQueueFamilies(physicalDevice, surface);

    uint32_t queueFamilyIndices[] = {
        queueIndices.graphics.value(), queueIndices.present.value()
    };

    if(queueFamilyIndices[0] == queueFamilyIndices[1]) {
        // Graphics and Present are the same queue
        swapchainInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        swapchainInfo.queueFamilyIndexCount = 0;
        swapchainInfo.pQueueFamilyIndices = nullptr;
    } else {
        // Graphics and Present are NOT the same queue
        swapchainInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        swapchainInfo.queueFamilyIndexCount = 2;
        swapchainInfo.pQueueFamilyIndices = queueFamilyIndices;
    }
    /*
        Image sharing modes:
        - EXCLUSIVE -> An image is owned by one queue family
            at a time and ownership must be manually transferred, 
            best performance
        - CONCURRENT -> An image can be accessed by many queue
            family concurrently 
    */

    swapchainInfo.preTransform = 
        swapchainDetails.capabilites.currentTransform;

    swapchainInfo.oldSwapchain = VK_NULL_HANDLE;

    if(vkCreateSwapchainKHR(
        device, &swapchainInfo, nullptr, &swapchain
    ) != VK_SUCCESS) {
        throw std::runtime_error("Error when creating Swapchain!");
    }
}