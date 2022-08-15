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

    VkCommandPool commandPool;
    VkCommandBuffer commandBuffer;

    // Queues
    VkQueue graphicsQueue;
    VkQueue presentQueue;
private:
    VkInstance instance;

    VkPhysicalDevice physicalDevice;

    VkSurfaceKHR surface;

    std::vector<VkImageView> imageViews;
    std::vector<VkFramebuffer> frameBuffers;

    // Debug
    VkDebugUtilsMessengerEXT debugMessenger;
    std::vector<const char*> activeLayers;

    QueueFamilyIndices familyIndices;

public:
    VulkanContext() = delete;
    VulkanContext(GLFWwindow* window);
    ~VulkanContext();

    // For now, we'll leave this here
    void RecordCommand(VkCommandBuffer&, uint32_t);

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
    void CreateCommandPool();
    void AllocateCommandBuffer();
    // Debug
    void CreateDebugMessenger();
    void PopulateDebugMessenger(VkDebugUtilsMessengerCreateInfoEXT&);
};