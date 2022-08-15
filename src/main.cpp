#include "GLFW/glfw3.h"

#include "api/vkcontext.hpp"
#include "utils/debug.hpp"

class VulkanApp {
private:
    GLFWwindow* window;
    VulkanContext context;

    VkSemaphore imageAvailableSemaphore, renderFinishedSemaphore;
    VkFence inFlightFence;

public:
    VulkanApp(const char* title, int width, int height):
        window(glfwCreateWindow(width, height, title, nullptr, nullptr)),
        context(window) {
        CreateSyncObjects();
    }

    ~VulkanApp() {
        vkDestroySemaphore(context.device, imageAvailableSemaphore, nullptr);
        vkDestroySemaphore(context.device, renderFinishedSemaphore, nullptr);
        vkDestroyFence(context.device, inFlightFence, nullptr);

        glfwDestroyWindow(window);
        glfwTerminate();
    }

    void Run() {
        while(!glfwWindowShouldClose(window)) {
            glfwPollEvents();
            Render();
        }
    }

    void Render() {
        // We wait for the last frame to have finished
        // We pass in an array of Fences, as well as it's size
        // The last two params refer to if we want to wait for all fences in
        // the array, and the timeout for each
        vkWaitForFences(context.device, 1, &inFlightFence, VK_TRUE, UINT64_MAX);
        // Since Fences are host sync objects, it's up to us to reset them
        vkResetFences(context.device, 1, &inFlightFence);

        uint32_t imageIndex = 0;

        // We acquire the next image index and store it
        // We pass in the device, the swapchain and the timeout
        // We can also pass in two sync objects - a semaphore and a fence -
        // for the API to signal after the image is done loading.
        vkAcquireNextImageKHR(
            context.device, context.swapchain.chain, UINT64_MAX,
            imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex
        );

        vkResetCommandBuffer(context.commandBuffer, 0);
        context.RecordCommand(context.commandBuffer, imageIndex);

        // We prepare to submit the command buffer
        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        // We specify which semaphores we're waiting on, as well
        // as which stages of the pipeline to wait
        // In our example, we want to write out color, so we must wait
        // for that stage to become available

        VkSemaphore waitSemaphores[]      = {imageAvailableSemaphore};
        VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;

        // We submit an array of our command buffers
        VkCommandBuffer commandBuffers[] = { context.commandBuffer };
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = commandBuffers;

        // We also pass in semaphores to be signaled when the
        // render finishes
        VkSemaphore signalSemaphores[] = { renderFinishedSemaphore };

        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;

        // We finally submit to the queue, passing in which queue to submit it to,
        // an array of submit infos and a fence to be signaled when execution finishes
        VK_ASSERT(vkQueueSubmit(context.graphicsQueue, 1, &submitInfo, inFlightFence));

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
        presentInfo.pResults = nullptr;

        // After that, we're finally ready to show
        // the world what we've done
        VK_ASSERT(
            vkQueuePresentKHR(context.graphicsQueue, &presentInfo)
        );
        
    }
private:

    void CreateSyncObjects() {
        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        // We add this flag so that the fence will start as signaled
        // This is to prevent and infinite wait time in our loop, since
        // we wait for this fence to signal in order to proceed
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        VK_ASSERT(vkCreateSemaphore(context.device, &semaphoreInfo, nullptr, &imageAvailableSemaphore));
        VK_ASSERT(vkCreateSemaphore(context.device, &semaphoreInfo, nullptr, &renderFinishedSemaphore));
        VK_ASSERT(vkCreateFence(context.device, &fenceInfo, nullptr, &inFlightFence));
    }
};

int main() {
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    VulkanApp app("Oi", 500, 500);

    app.Run();
}