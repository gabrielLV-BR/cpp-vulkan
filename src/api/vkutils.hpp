#pragma once

#include <vector>
#include <iostream>

#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"

namespace VkUtils {
    std::vector<const char*> GetExtensions();
    void CheckLayers();

    bool IsDeviceSuitable(VkPhysicalDevice device);

    // Debug message callback
    VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData
    );

    VkResult CreateDebugMessenger(
        VkInstance instance,
        const VkDebugUtilsMessengerCreateInfoEXT* info,
        const VkAllocationCallbacks* alloc,
        VkDebugUtilsMessengerEXT* messenger
    );

    VkResult DestroyDebugMessenger(
        VkInstance instance, 
        VkDebugUtilsMessengerEXT debugMessenger, 
        const VkAllocationCallbacks* pAllocator
    );
}