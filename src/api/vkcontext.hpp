#pragma once

#include "vulkan/vulkan.h"
#include "GLFW/glfw3.h"
#include <vector>

const std::vector<const char*> VALIDATION_LAYERS = {
    "VK_LAYER_KHRONOS_validation",
    "VK_LAYER_LUNARG_standard_validation"
};

#ifdef NDEBUG
    const bool useValidationLayers = false;
#else
    const bool useValidationLayers = true;
#endif


class VulkanContext {
private:
    VkInstance instance;
    VkPhysicalDevice physicalDevice;
    VkDevice device;
    VkDebugUtilsMessengerEXT debugMessenger;
public:
    VulkanContext();
    VulkanContext(GLFWwindow* window);
    void Destroy();

private:
    void CreateInstance();
    void PickPhysicalDevice();
    // Debug
    void CreateDebugMessenger();
    void PopulateDebugMessenger(VkDebugUtilsMessengerCreateInfoEXT&);
};