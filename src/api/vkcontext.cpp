#include "vkcontext.hpp"

#include <vulkan/vulkan.h>

#include <limits>
#include <set>
#include <stdexcept>

#include "utils/debug.hpp"
#include "utils/math.hpp"
#include "vkutils.hpp"
#include "./components/vkswapchain.hpp"

VulkanContext::VulkanContext( GLFWwindow* window )
{
  CreateInstance();
  CreateDebugMessenger();
  CreateSurface( window );
  PickPhysicalDevice();
  CreateLogicalDevice();

  pipeline = Pipeline( device );
  swapchain = Swapchain(
    window,
    device,
    physicalDevice,
    surface
  );

  pipeline.CreateRenderPass( device, swapchain.format );
  pipeline.CreatePipeline( device, GetViewport(), GetScissor() );

  swapchain.CreateImageViews(device);
  swapchain.CreateFrameBuffers(device, pipeline.renderPass);
}

VulkanContext::~VulkanContext()
{
  if ( useValidationLayers ) {
    VkUtils::DestroyDebugMessenger( instance, debugMessenger, nullptr );
  }

  pipeline.Destroy( device );
  swapchain.Destroy( device );

  vkDestroyDevice( device, nullptr );
  vkDestroySurfaceKHR( instance, surface, nullptr );
  vkDestroyInstance( instance, nullptr );
}

void VulkanContext::CreateInstance()
{
  VkApplicationInfo appInfo{};
  appInfo.pApplicationName = "My Vulkan App";
  appInfo.applicationVersion = VK_MAKE_VERSION( 1, 0, 0 );

  appInfo.pEngineName = "My Engine";
  appInfo.engineVersion = VK_MAKE_VERSION( 1, 0, 0 );

  appInfo.apiVersion = VK_API_VERSION_1_0;
  appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;

  VkInstanceCreateInfo instanceInfo{};
  instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  instanceInfo.pApplicationInfo = &appInfo;

  // Setting up debugging
  VkDebugUtilsMessengerCreateInfoEXT dMessenger{};
  PopulateDebugMessenger( dMessenger );
  instanceInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&dMessenger;
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

  VK_ASSERT( vkCreateInstance( &instanceInfo, nullptr, &instance ) );
}

void VulkanContext::CreateSurface( GLFWwindow* window )
{
  VK_ASSERT( glfwCreateWindowSurface( instance, window, nullptr, &surface ) );
}

void VulkanContext::CreateDebugMessenger()
{
  VkDebugUtilsMessengerCreateInfoEXT info{};
  PopulateDebugMessenger( info );

  VK_ASSERT(
    VkUtils::CreateDebugMessenger( 
      instance, 
      &info, 
      nullptr,
      &debugMessenger 
    ) 
  );
}

void VulkanContext::PopulateDebugMessenger(
    VkDebugUtilsMessengerCreateInfoEXT& debugMessenger )
{
  debugMessenger.sType = 
    VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
  debugMessenger.pfnUserCallback = VkUtils::DebugCallback;
  debugMessenger.messageSeverity = 
    VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT
    | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
    | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT;

  debugMessenger.messageType = 
    VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
    | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT
    | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
  debugMessenger.pUserData = nullptr;
  debugMessenger.pNext = nullptr;
}

void VulkanContext::PickPhysicalDevice()
{
  physicalDevice = VK_NULL_HANDLE;
  uint32_t physicalDeviceCount = 0;
  vkEnumeratePhysicalDevices( 
    instance, 
    &physicalDeviceCount,
    nullptr
  );

  if ( physicalDeviceCount == 0 ) {
    throw std::runtime_error(
      "No available Physical Devices with Vulkan support"
    );
  }

  std::vector<VkPhysicalDevice> physicalDevices( physicalDeviceCount );
  vkEnumeratePhysicalDevices( 
    instance, 
    &physicalDeviceCount,
    physicalDevices.data() 
  );

  for ( auto device : physicalDevices ) {
    if ( VkUtils::IsDeviceSuitable( device, surface ) ) {
      physicalDevice = device;
      break;
    }
  }

  if ( physicalDevice == VK_NULL_HANDLE ) {
    throw std::runtime_error( "No suitable Physical Devices found" );
  }
}

void VulkanContext::CreateLogicalDevice()
{
  familyIndices = VkUtils::FindQueueFamilies( 
    physicalDevice, 
    surface
  );

  // Since we have multiple queues, we must provide an
  // array of queue infos.
  // As shown in LearnVulkan.com, a `set` is a nice way
  // to make this more compact AND remove duplicate indices
  // (which is likely to happen with `graphics` and `present`)

  using std::set;
  using std::vector;

  vector<VkDeviceQueueCreateInfo> queueCreateInfos;

  set<uint32_t> uniqueQueueFamilies = { 
    familyIndices.graphics.value(), 
    familyIndices.present.value() 
  };

  float queuePriority = 1.0f;
  for ( uint32_t queueFamily : uniqueQueueFamilies ) {
    VkDeviceQueueCreateInfo queueCreateInfo{};
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = queueFamily;
    queueCreateInfo.queueCount = 1;
    queueCreateInfo.pQueuePriorities = &queuePriority;

    queueCreateInfos.push_back( queueCreateInfo );
  }

  // They are all initialized to VK_FALSE like this
  VkPhysicalDeviceFeatures features{};

  // Device
  VkDeviceCreateInfo deviceInfo{};

  deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

  deviceInfo.queueCreateInfoCount
      = static_cast<uint32_t>( queueCreateInfos.size() );
  deviceInfo.pQueueCreateInfos = queueCreateInfos.data();
  deviceInfo.pEnabledFeatures = &features;

  deviceInfo.enabledExtensionCount = static_cast<uint32_t>(
      DEVICE_EXTENSIONS.size() );  // We'll come back for these
  deviceInfo.ppEnabledExtensionNames = DEVICE_EXTENSIONS.data();

  // In modern Vulkan, Device layers are ignored, as there's
  // no longer a distinction between Device and Instance layers
  // It is still, however, a good idea to set them as it allows for
  // backwards compatibility
  if ( useValidationLayers ) {
    deviceInfo.enabledLayerCount = activeLayers.size();
    deviceInfo.ppEnabledLayerNames = activeLayers.data();
  }
  else {
    deviceInfo.enabledLayerCount = 0;
  }

  VK_ASSERT( vkCreateDevice( physicalDevice, &deviceInfo, nullptr, &device ) );

  vkGetDeviceQueue( 
    device, 
    familyIndices.graphics.value(),
    0 /* because we're only using one queue for this family */,
    &graphicsQueue
  );

  vkGetDeviceQueue( 
    device, 
    familyIndices.present.value(),
    0 /* because we're only using one queue for this family */,
    &presentQueue
  );
}

VkViewport VulkanContext::GetViewport()
{
  return VkViewport{ 
    .x = 0.0f,
    .y = 0.0f,
    .width = static_cast<float>( swapchain.extent.width ),
    .height = static_cast<float>( swapchain.extent.height ),
    .minDepth = 0.0f,
    .maxDepth = 1.0f
  };
}

VkRect2D VulkanContext::GetScissor()
{
  return VkRect2D{ 
    .offset = { 0, 0 }, 
    .extent = swapchain.extent
  };
}