cmake_minimum_required(VERSION 3.17)

project(LearningVulkan)

add_subdirectory(modules/glfw)

add_executable(
    ${PROJECT_NAME}
    "src/main.cpp"
    "src/api/vkcontext.cpp"
    "src/api/vkutils.cpp"
    "src/utils/option.hpp"
)

target_link_libraries(${PROJECT_NAME} PRIVATE glfw vulkan)
target_include_directories(${PROJECT_NAME} PRIVATE modules/ src/)