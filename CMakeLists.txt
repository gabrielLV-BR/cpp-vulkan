cmake_minimum_required(VERSION 3.17)

project(LearningVulkan)

add_subdirectory(modules/glfw)

add_executable(
    ${PROJECT_NAME}
    "src/main.cpp"
    "src/api/vkcontext.cpp"
    "src/api/vkutils.cpp"
    "src/api/components/vkshader.cpp"
    "src/api/components/vkpipeline.cpp"
    "src/utils/option.hpp"
    "src/utils/file.hpp"
)

target_compile_definitions(
    ${PROJECT_NAME} PRIVATE
    RESOURCES="${CMAKE_SOURCE_DIR}/src/resources/"
)

target_link_libraries(${PROJECT_NAME} PRIVATE glfw vulkan)
target_include_directories(${PROJECT_NAME} PRIVATE modules/ src/)