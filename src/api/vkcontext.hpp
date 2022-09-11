#pragma once

#include <vulkan/vulkan.h>
#include "GLFW/glfw3.h"

#include "components/vkpipeline.hpp"

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

const std::vector<const char*> DEVICE_EXTENSIONS = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

struct Swapchain {
    VkSwapchainKHR chain;
    std::vector<VkImage> images;
    VkFormat format;
    VkExtent2D extent;

    void Destroy(VkDevice device) {
        vkDestroySwapchainKHR(device, chain, nullptr);
    }
};

class VulkanContext {
public:
    VkDevice device;
    Swapchain swapchain;
    Pipeline pipeline;

    // Queues
    VkQueue graphicsQueue;
    VkQueue presentQueue;

    std::vector<VkFramebuffer> frameBuffers;
    QueueFamilyIndices familyIndices;

private:
    VkInstance instance;
    VkPhysicalDevice physicalDevice;
    VkSurfaceKHR surface;

    std::vector<VkImageView> imageViews;

    // Debug
    VkDebugUtilsMessengerEXT debugMessenger;
    std::vector<const char*> activeLayers;

public:
    VulkanContext() = delete;
    VulkanContext(GLFWwindow* window);
    ~VulkanContext();

    VkViewport GetViewport();
    VkRect2D GetScissor();

private:
    void CreateInstance();
    void PickPhysicalDevice();
    void CreateLogicalDevice();
    void GetQueues();
    void CreateSurface(GLFWwindow*);
    void CreateSwapchain(GLFWwindow*);
    void CreateImageView();
    void CreatePipeline();
    void CreateFramebuffers();
    // Debug
    void CreateDebugMessenger();
    void PopulateDebugMessenger(VkDebugUtilsMessengerCreateInfoEXT&);
};