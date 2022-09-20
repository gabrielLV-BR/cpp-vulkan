#include <vulkan/vulkan.h>

#include "./vkswapchain.hpp"

#include "../vkcontext.hpp"
#include "../vkutils.hpp"
#include "../../utils/math.hpp"
#include "../../utils/debug.hpp"

#include <set>
#include <limits>
#include <stdexcept>

Swapchain::Swapchain() 
  : handle(VK_NULL_HANDLE), format(VK_FORMAT_UNDEFINED), extent({}) 
  {}

Swapchain::Swapchain(
  GLFWwindow *window, 
  VkDevice device,
  VkPhysicalDevice physicalDevice,
  VkSurfaceKHR surface
) {

  auto swapchainDetails = VkUtils::FindSwapchainSupport(
    physicalDevice,
    surface
  );

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
    for (auto &format : swapchainDetails.formats)
    {
      if (format.format == VK_FORMAT_R8G8B8A8_SRGB &&
        format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
      {
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
    for (auto &mode : swapchainDetails.presentModes)
    {
      if (
        mode == VK_PRESENT_MODE_MAILBOX_KHR ||
        mode == VK_PRESENT_MODE_FIFO_KHR)
      {
        presentMode = mode;
        break;
      }
    }
  }

  { // Extent2D
    auto &cap = swapchainDetails.capabilites;
    extent = cap.currentExtent;

    if (
      cap.currentExtent.width ==
      std::numeric_limits<uint32_t>::max())
    {
      int width, height;

      glfwGetFramebufferSize(window, &width, &height);

      extent = {
        MathUtils::clamp(
          static_cast<uint32_t>(width),
          cap.minImageExtent.width,
          cap.maxImageExtent.width),
        MathUtils::clamp(
          static_cast<uint32_t>(height),
          cap.minImageExtent.height,
          cap.maxImageExtent.height)};
    }
  }

  swapchainInfo.imageExtent = extent;
  swapchainInfo.presentMode = presentMode;
  swapchainInfo.imageFormat = surfaceFormat.format;
  swapchainInfo.imageColorSpace = surfaceFormat.colorSpace;

  auto imageCount = MathUtils::clamp(
    swapchainDetails.capabilites.minImageCount + 1,
    swapchainDetails.capabilites.minImageCount,
    swapchainDetails.capabilites.maxImageCount);

  swapchainInfo.minImageCount = imageCount;
  swapchainInfo.imageArrayLayers = 1;
  swapchainInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

  auto queueIndices = VkUtils::FindQueueFamilies(
    physicalDevice, 
    surface
  );

  uint32_t queueFamilyIndices[] = {
    queueIndices.graphics.value(), queueIndices.present.value()};

  /*
    Image sharing modes:
    - EXCLUSIVE -> An image is owned by one queue family
      at a time and ownership must be manually transferred,
      best performance
    - CONCURRENT -> An image can be accessed by many queue
      family concurrently
  */
  if (queueFamilyIndices[0] == queueFamilyIndices[1])
  {
    // Graphics and Present are the same queue
    swapchainInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swapchainInfo.queueFamilyIndexCount = 0;
    swapchainInfo.pQueueFamilyIndices = nullptr;
  }
  else
  {
    // Graphics and Present are NOT the same queue
    swapchainInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
    swapchainInfo.queueFamilyIndexCount = 2;
    swapchainInfo.pQueueFamilyIndices = queueFamilyIndices;
  }
  swapchainInfo.preTransform =
      swapchainDetails.capabilites.currentTransform;

  swapchainInfo.oldSwapchain = VK_NULL_HANDLE;

  VK_ASSERT(
    vkCreateSwapchainKHR(
      device, &swapchainInfo, nullptr, &handle
    )
  );
  
  // Now, load all the images in that habitual way

  uint32_t swapchainImageCount;

  vkGetSwapchainImagesKHR(
    device, 
    handle, 
    &swapchainImageCount, 
    nullptr
  );
  
  images.resize(swapchainImageCount);

  vkGetSwapchainImagesKHR(
    device,
    handle,
    &swapchainImageCount,
    images.data()
  );

  format = surfaceFormat.format;
}

void Swapchain::Destroy(VkDevice device)
{
  for(auto& frameBuffer : frameBuffers) {
    vkDestroyFramebuffer(device, frameBuffer, nullptr);
  }

  for(auto& imageView : imageViews) {
    vkDestroyImageView(device, imageView, nullptr);
  }
}

void Swapchain::CreateImageViews(VkDevice device) {
  imageViews.resize(images.size());

  for(int i = 0; i < imageViews.size(); i++) {
    VkImageViewCreateInfo imageInfo{};

    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    imageInfo.format = format;
    imageInfo.image = images[i];
    imageInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;

    imageInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    imageInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    imageInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    imageInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

    imageInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    imageInfo.subresourceRange.layerCount = 1;
    imageInfo.subresourceRange.levelCount = 1;
    imageInfo.subresourceRange.baseMipLevel = 0;
    imageInfo.subresourceRange.baseArrayLayer = 0;

    VK_ASSERT(
      vkCreateImageView(
        device, 
        &imageInfo, 
        nullptr, 
        &imageViews[i]
      )
    );
  }
}

void Swapchain::CreateFrameBuffers(
  VkDevice device,
  VkRenderPass renderPass
) {
  // We must create a Framebuffer for each
  // imageView attachment
  frameBuffers.resize(imageViews.size());

  for(int i = 0; i < imageViews.size(); i++) {
    VkImageView attachments[] = {
      imageViews[i]
    };

    VkFramebufferCreateInfo frameBufferInfo{};
    frameBufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;

    frameBufferInfo.attachmentCount = 1;
    frameBufferInfo.pAttachments = attachments;
    frameBufferInfo.layers = 1;

    frameBufferInfo.width = extent.width;
    frameBufferInfo.height = extent.height;

    frameBufferInfo.renderPass = renderPass;

    VK_ASSERT(
      vkCreateFramebuffer(
        device, 
        &frameBufferInfo, 
        nullptr,
        &frameBuffers[i]
      )
    );
  }
}