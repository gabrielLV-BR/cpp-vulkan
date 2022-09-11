#include <vulkan/vulkan.h>
#include "GLFW/glfw3.h"

#include "api/vkcontext.hpp"
#include "utils/debug.hpp"

#include <vector>

using std::vector;

class VulkanApp {
private:
    GLFWwindow* window;
    VulkanContext context;

    /*
        Frames in Flight refers to how the CPU can process a frame
        while the GPU is rendering another one. If it weren't for
        these, the CPU would have to idle while the GPU renders the 
        last frame. We can make it so that the CPU keeps working.
        This improves performance, however, can introduce latency
        if we were to allow too many frames to be in flight.
        2 is a good number.
    */
    const int MAX_FRAMES_IN_FLIGHT = 2;

    /*
        If we're processing future frames, we can't just use the
        same objects as the current frame is, as Vulkan is reading
        and writing to them. We must create multiple sync objects
        and commands buffers, in order to leave the objects being
        used alone
    */
    vector<VkSemaphore> imageAvailableSemaphores, renderFinishedSemaphores;
    vector<VkFence> inFlightFences;

    VkCommandPool commandPool;
    vector<VkCommandBuffer> commandBuffers;

    uint32_t currentFrame;

public:
    VulkanApp(const char* title, int width, int height):
        window(glfwCreateWindow(width, height, title, nullptr, nullptr)),
        context(window), currentFrame(0),
        renderFinishedSemaphores(MAX_FRAMES_IN_FLIGHT),
        imageAvailableSemaphores(MAX_FRAMES_IN_FLIGHT),
        commandBuffers(MAX_FRAMES_IN_FLIGHT)
    {
        CreateCommandPool();
        AllocateCommandBuffers();
        CreateSyncObjects();
    }

    ~VulkanApp() {
        for(int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            vkDestroySemaphore(context.device, imageAvailableSemaphores[i], nullptr);
            vkDestroySemaphore(context.device, renderFinishedSemaphores[i], nullptr);
            vkDestroyFence(context.device, inFlightFences[i], nullptr);
        }

        // All commandBuffers contained on this commandPool get freed
        // automatically
        vkDestroyCommandPool(context.device, commandPool, nullptr);

        glfwDestroyWindow(window);
        glfwTerminate();
    }

    void Run() {
        while(!glfwWindowShouldClose(window)) {
            glfwPollEvents();
            Render();
            currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
        }
        vkDeviceWaitIdle(context.device);
    }

    void Render() {
        // We wait for the last frame to have finished
        // We pass in an array of Fences, as well as it's size
        // The last two params refer to if we want to wait for all fences in
        // the array, and the timeout for each
        vkWaitForFences(context.device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);
        // Since Fences are host sync objects, it's up to us to reset them
        vkResetFences(context.device, 1, &inFlightFences[currentFrame]);

        uint32_t imageIndex = 0;

        // We acquire the next image index and store it
        // We pass in the device, the swapchain and the timeout
        // We can also pass in two sync objects - a semaphore and a fence -
        // for the API to signal after the image is done loading.
        vkAcquireNextImageKHR(
            context.device, context.swapchain.chain, UINT64_MAX,
            imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex
        );

        vkResetCommandBuffer(commandBuffers[currentFrame], 0);
        RecordCommand(commandBuffers[currentFrame], imageIndex);

        // We prepare to submit the command buffer
        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        // We specify which semaphores we're waiting on, as well
        // as which stages of the pipeline to wait
        // In our example, we want to write out color, so we must wait
        // for that stage to become available

        VkSemaphore waitSemaphores[]      = {imageAvailableSemaphores[currentFrame]};
        VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;

        // We submit an array of our command buffers
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffers[currentFrame];

        // We also pass in semaphores to be signaled when the
        // render finishes
        VkSemaphore signalSemaphores[] = { renderFinishedSemaphores[currentFrame] };

        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;

        // We finally submit to the queue, passing in which queue to submit it to,
        // an array of submit infos and a fence to be signaled when execution finishes
        VK_ASSERT(vkQueueSubmit(context.graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]));

        // After we rendered onto the attachment, we must
        // present it back to the swapchain in order to
        // see the results
        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        
        // We wait on the semaphores that the QueueSubmit
        // will signal when done
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = signalSemaphores;

        // We pass in the image index...
        presentInfo.pImageIndices = &imageIndex;

        // ...and the swapchains
        VkSwapchainKHR swapchains[] = {context.swapchain.chain};
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = swapchains;

        // If we had used multiple swapchains, we
        // would pass and array of VkResults to
        // verify the output of each one
        // Not necessary when using just one swapchain
        // presentInfo.pResults = nullptr;

        // After that, we're finally ready to show
        // the world what we've done
        VK_ASSERT(
            vkQueuePresentKHR(context.graphicsQueue, &presentInfo)
        );
    }
private:

    void CreateCommandPool() {
        VkCommandPoolCreateInfo commandPoolInfo{};
        commandPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        // These flags allow us to specify how we're going to use the
        // command buffers
        // Since we're going to be re-recording them each frame, we specify
        // the CREATE_RESET bit, as to make Vulkan optimize for frequent records 
        commandPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        commandPoolInfo.queueFamilyIndex = context.familyIndices.graphics.value();

        VK_ASSERT(
            vkCreateCommandPool(context.device, &commandPoolInfo, nullptr, &commandPool)
        );
    }

    void AllocateCommandBuffers() {
        VkCommandBufferAllocateInfo commandBufferInfo{};
        commandBufferInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        // A Command Buffer's level can be:
        // - PRIMARY: It's passed directly to the command queue
        // - SECONDARY: Can only be called from primary command 
        commandBufferInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        commandBufferInfo.commandPool = commandPool;

        commandBuffers.resize(2);
        // Here we specify the ammount of command buffers we will
        // allocate VkAllocateCommandBuffers
        commandBufferInfo.commandBufferCount = commandBuffers.size();
        VK_ASSERT(
            vkAllocateCommandBuffers(context.device, &commandBufferInfo, &commandBuffers[currentFrame])
        );
    }

    void RecordCommand(VkCommandBuffer& command, uint32_t imageIndex) {
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
        passBeginInfo.renderArea.extent = context.swapchain.extent;
        passBeginInfo.renderArea.offset = { 0, 0 };
        //
        passBeginInfo.renderPass = context.pipeline.renderPass;
        passBeginInfo.framebuffer = context.frameBuffers[imageIndex];

        VkClearValue clearColor {{{0.2f, 0.2f, 0.2f, 1.0f}}};

        passBeginInfo.clearValueCount = 1;
        passBeginInfo.pClearValues = &clearColor;

        // The last parameter has to do with wheter we're gonna use secondary
        // command buffers or not.
        // If yes, we should use VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS
        // If not, we should use VK_SUBPASS_CONTENTS_INLINE
        vkCmdBeginRenderPass(command, &passBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

        // All drawing commands begin with vkCmd*** and all return void
        vkCmdBindPipeline(command, VK_PIPELINE_BIND_POINT_GRAPHICS, context.pipeline.pipeline);

        // VkViewport viewports[] = { GetViewport() };
        // VkRect2D scissors[] = { GetScissor() };

        VkViewport viewport = context.GetViewport();
        vkCmdSetViewport(command, 0, 1, &viewport);

        VkRect2D scissor = context.GetScissor();
        vkCmdSetScissor(command, 0, 1, &scissor);

        /* The param names are really self-explanatory
            commandBuffer: the comand buffer
            vertexCount: number of vertices (baked into shader, for now)
            instanceCount: number of instances
            firstVertex: starting vertex (defines lowest value of gl_VertexIndex)
            firstInstance: starting instance (defines lowest value of gl_InstanceIndex) */
        vkCmdDraw(command, 3, 1, 0, 0);

        vkCmdEndRenderPass(command);

        VK_ASSERT(
            vkEndCommandBuffer(command)
        );
    }

    void CreateSyncObjects() {
        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        // We add this flag so that the fence will start as signaled
        // This is to prevent and infinite wait time in our loop, since
        // we wait for this fence to signal in order to proceed
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

        for(int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            VK_ASSERT(vkCreateSemaphore(context.device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]));
            VK_ASSERT(vkCreateSemaphore(context.device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]));
            VK_ASSERT(vkCreateFence(context.device, &fenceInfo, nullptr, &inFlightFences[i]));
        }

    }
};

int main() {
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    VulkanApp app("Oi", 500, 500);

    app.Run();

    return 0;
}