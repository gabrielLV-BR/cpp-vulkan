#include <vulkan/vulkan.h>

#include "vkcontext.hpp"
#include "vkutils.hpp"
#include "utils/debug.hpp"
#include "utils/math.hpp"

#include <set>
#include <limits>
#include <stdexcept>

VulkanContext::VulkanContext(GLFWwindow *window)
{
    CreateInstance();
    CreateDebugMessenger();
    CreateSurface(window);
    PickPhysicalDevice();
    CreateLogicalDevice();
    CreateSwapchain(window);
    CreateImageView();
    CreatePipeline();
    CreateFramebuffers();
    CreateCommandPool();
    AllocateCommandBuffer();
}

VulkanContext::~VulkanContext()
{
    if (useValidationLayers)
    {
        VkUtils::DestroyDebugMessenger(instance, debugMessenger, nullptr);
    }

    for(auto& frameBuffer : frameBuffers) {
        vkDestroyFramebuffer(device, frameBuffer, nullptr);
    }

    for(auto& imageView : imageViews) {
        vkDestroyImageView(device, imageView, nullptr);
    }

    vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
    vkDestroyCommandPool(device, commandPool, nullptr);

    pipeline.Destroy(device);
    swapchain.Destroy(device);
    
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

    appInfo.apiVersion = VK_API_VERSION_1_0;
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

    VK_ASSERT(
        vkCreateInstance(&instanceInfo, nullptr, &instance)
    );
}

void VulkanContext::CreateSurface(GLFWwindow *window)
{
    VK_ASSERT(
        glfwCreateWindowSurface(instance, window, nullptr, &surface)
    );
}

void VulkanContext::CreateDebugMessenger()
{
    VkDebugUtilsMessengerCreateInfoEXT info{};
    PopulateDebugMessenger(info);

    VK_ASSERT(
        VkUtils::CreateDebugMessenger(instance, &info, nullptr, &debugMessenger)
    );
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
    debugMessenger.pNext = nullptr;
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
    familyIndices = VkUtils::FindQueueFamilies(physicalDevice, surface);

    // Since we have multiple queues, we must provide an
    // array of queue infos.
    // As shown in LearnVulkan.com, a `set` is a nice way
    // to make this more compact AND remove duplicate indices
    // (which is likely to happen with `graphics` and `present`)

    using std::vector;
    using std::set;

    vector<VkDeviceQueueCreateInfo> queueCreateInfos;

    set<uint32_t> uniqueQueueFamilies = {
        familyIndices.graphics.value(), familyIndices.present.value()
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

    VK_ASSERT( 
        vkCreateDevice(
            physicalDevice, 
            &deviceInfo, 
            nullptr, 
            &device
        ) 
    );

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
                MathUtils::clamp(
                    static_cast<uint32_t>(width),
                    cap.minImageExtent.width,
                    cap.maxImageExtent.width
                ),
                MathUtils::clamp(
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

    auto imageCount = MathUtils::clamp(
        swapchainDetails.capabilites.minImageCount + 1,
        swapchainDetails.capabilites.minImageCount,
        swapchainDetails.capabilites.maxImageCount
    );

    swapchainInfo.minImageCount = imageCount;
    swapchainInfo.imageArrayLayers = 1;
    swapchainInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    // auto queueIndices = VkUtils::FindQueueFamilies(physicalDevice, surface);

    uint32_t queueFamilyIndices[] = {
        familyIndices.graphics.value(), familyIndices.present.value()
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

    VK_ASSERT(
        vkCreateSwapchainKHR(
            device, &swapchainInfo, nullptr, &swapchain.chain
        )
    );

    // Now, load all the images in that habitual way

    uint32_t swapchainImageCount;

    vkGetSwapchainImagesKHR(device, swapchain.chain, &swapchainImageCount, nullptr);
    swapchain.images.resize(swapchainImageCount);
    vkGetSwapchainImagesKHR(
        device, 
        swapchain.chain, 
        &swapchainImageCount, 
        swapchain.images.data()
    );

    swapchain.extent = extent;
    swapchain.format = surfaceFormat.format;
}

void VulkanContext::CreateImageView() {
    imageViews.resize(swapchain.images.size());

    for(int i = 0; i < imageViews.size(); i++) {
        VkImageViewCreateInfo imageInfo{};

        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        imageInfo.format = swapchain.format;
        imageInfo.image = swapchain.images[i];
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

void VulkanContext::CreatePipeline() {
    pipeline = Pipeline(device);
    pipeline.CreateRenderPass(device, swapchain.format);
    pipeline.CreatePipeline(device, GetViewport(), GetScissor());
}

void VulkanContext::CreateFramebuffers() {
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

        frameBufferInfo.width = swapchain.extent.width;
        frameBufferInfo.height = swapchain.extent.height;

        frameBufferInfo.renderPass = pipeline.renderPass;

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

void VulkanContext::CreateCommandPool() {
    VkCommandPoolCreateInfo commandPoolInfo{};
    commandPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    // These flags allow us to specify how we're going to use the
    // command buffers
    // Since we're going to be re-recording them each frame, we specify
    // the CREATE_RESET bit, as to make Vulkan optimize for frequent records 
    commandPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    commandPoolInfo.queueFamilyIndex = familyIndices.graphics.value();

    VK_ASSERT(
        vkCreateCommandPool(device, &commandPoolInfo, nullptr, &commandPool)
    );
}

void VulkanContext::AllocateCommandBuffer() {
    VkCommandBufferAllocateInfo commandBufferInfo{};
    commandBufferInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    // A Command Buffer's level can be:
    // - PRIMARY: It's passed directly to the command queue
    // - SECONDARY: Can only be called from primary command 
    commandBufferInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    commandBufferInfo.commandPool = commandPool;

    // Here we specify the ammount of command buffers...
    commandBufferInfo.commandBufferCount = 1;
    //                         ...we're allocatting here in &commandBuffer
    VK_ASSERT(
        vkAllocateCommandBuffers(device, &commandBufferInfo, &commandBuffer)
    );
}

VkViewport VulkanContext::GetViewport() {
    return VkViewport {
        .x = 0.0f,
        .y = 0.0f,
        .width = static_cast<float>(swapchain.extent.width),
        .height = static_cast<float>(swapchain.extent.height),
        .minDepth = 0.0f,
        .maxDepth = 1.0f
    };
}

VkRect2D VulkanContext::GetScissor() {
    return VkRect2D {
        .offset = { 0, 0 },
        .extent = swapchain.extent
    };
}

void VulkanContext::RecordCommand(VkCommandBuffer& command, uint32_t imageIndex) {

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    // /* Optional */ beginInfo.flags            = 0; // Specify usage, won't matter for us right now
    // /* Optional */ beginInfo.pInheritanceInfo = nullptr; // Specify primary buffer to inherit (only for secondary buffers)

    VK_ASSERT(
        vkBeginCommandBuffer(command, &beginInfo)
    );

    VkRenderPassBeginInfo passBeginInfo{};
    passBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    // Render area
    passBeginInfo.renderArea.extent = swapchain.extent;
    passBeginInfo.renderArea.offset = { 0, 0 };
    //
    passBeginInfo.renderPass = pipeline.renderPass;
    passBeginInfo.framebuffer = frameBuffers[imageIndex];

    VkClearValue clearColor {{{0.2f, 0.2f, 0.2f, 1.0f}}};

    passBeginInfo.clearValueCount = 1;
    passBeginInfo.pClearValues = &clearColor;

    // The last parameter has to do with wheter we're gonna use secondary
    // command buffers or not.
    // If yes, we should use VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS
    // If not, we should use VK_SUBPASS_CONTENTS_INLINE
    vkCmdBeginRenderPass(command, &passBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

    // All drawing commands begin with vkCmd*** and all return void
    vkCmdBindPipeline(command, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.pipeline);

    // VkViewport viewports[] = { GetViewport() };
    // VkRect2D scissors[] = { GetScissor() };

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(swapchain.extent.width);
    viewport.height = static_cast<float>(swapchain.extent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(command, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = { 0, 0 };
    scissor.extent = swapchain.extent;
    vkCmdSetScissor(command, 0, 1, &scissor);

    /* The param names are really self-explanatory
        commandBuffer: the comand buffer
    //! vertexCount: number of vertices (baked into shader, for now)
        instanceCount: number of instances
        firstVertex: starting vertex (defines lowest value of gl_VertexIndex)
        firstInstance: starting instance (defines lowest value of gl_InstanceIndex)
    */
    vkCmdDraw(command, 3, 1, 0, 0);

    vkCmdEndRenderPass(command);

    VK_ASSERT(
        vkEndCommandBuffer(command)
    );
}
