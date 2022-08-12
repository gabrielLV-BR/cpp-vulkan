#pragma once

#include <vulkan/vulkan.h>
#include "GLFW/glfw3.h"
#include <vector>

#include "components/vkpipeline.hpp"

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
private:
    VkInstance instance;

    VkDevice device;
    VkPhysicalDevice physicalDevice;

    Pipeline pipeline;

    VkSurfaceKHR surface;
    Swapchain swapchain;

    VkCommandPool commandPool;
    VkCommandBuffer commandBuffer;

    std::vector<VkImageView> imageViews;
    std::vector<VkFramebuffer> frameBuffers;

    // Queues
    VkQueue graphicsQueue;
    VkQueue presentQueue;

    // Debug
    VkDebugUtilsMessengerEXT debugMessenger;
    std::vector<const char*> activeLayers;

    QueueFamilyIndices familyIndices;

public:
    VulkanContext();
    VulkanContext(GLFWwindow* window);
    void Destroy();

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
    void CreateCommandPool();
    void AllocateCommandBuffer();
    // Debug
    void CreateDebugMessenger();
    void PopulateDebugMessenger(VkDebugUtilsMessengerCreateInfoEXT&);
};