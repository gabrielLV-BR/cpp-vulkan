#pragma once

#include <vector>
#include <iostream>
#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"

#include "utils/option.hpp"

using std::experimental::optional;
struct QueueFamilyIndices {
    optional<uint32_t> graphics;
    optional<uint32_t> present;

    QueueFamilyIndices() {}

    bool IsComplete() {
        return graphics.has_value() && present.has_value();
    }
};

namespace VkUtils {
    std::vector<const char*> GetExtensions();
    std::vector<const char*> GetLayers();

    bool IsDeviceSuitable(VkPhysicalDevice device, VkSurfaceKHR surface);

    QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface);

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