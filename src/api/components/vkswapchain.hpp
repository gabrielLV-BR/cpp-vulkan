#pragma once

#include <vulkan/vulkan.h>

#include <vector>

struct Swapchain {
  VkSwapchainKHR handle;

  std::vector<VkImage> images;
  std::vector<VkImageView> imageViews;
  std::vector<VkFramebuffer> frameBuffers;

  VkFormat format;
  VkExtent2D extent;

  Swapchain(
    GLFWwindow *window,
    VkDevice device,
    VkPhysicalDevice phyisicalDevice,
    VkSurfaceKHR surface
  );

  void Destroy(VkDevice device);

  void CreateFrameBuffers(VkDevice, VkRenderPass);
  void CreateImageViews(VkDevice);
};