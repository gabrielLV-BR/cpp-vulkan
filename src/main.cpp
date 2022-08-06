#include "GLFW/glfw3.h"

#include "api/vkcontext.hpp"

class VulkanApp {
private:
    GLFWwindow* window;
    VulkanContext context;
public:
    VulkanApp(const char* title, int width, int height) {
        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

        window = glfwCreateWindow(width, height, title, nullptr, nullptr);
        context = VulkanContext(window);
    }

    ~VulkanApp() {
        glfwDestroyWindow(window);
        glfwTerminate();
    }

    void Run() {
        while(!glfwWindowShouldClose(window)) {
            glfwPollEvents();
        }
    }
};

int main() {
    VulkanApp app("Oi", 500, 500);
    app.Run();
}