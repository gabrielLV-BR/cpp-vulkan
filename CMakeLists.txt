cmake_minimum_required(VERSION 3.17)

project(LearningVulkan)

add_subdirectory(modules/glfw)

add_executable(
    ${PROJECT_NAME}
    "${CMAKE_SOURCE_DIR}/src/main.cpp"
    "${CMAKE_SOURCE_DIR}/src/api/vkcontext.cpp"
    "${CMAKE_SOURCE_DIR}/src/api/vkcontext.hpp"
    "${CMAKE_SOURCE_DIR}/src/api/vkutils.cpp"
    "${CMAKE_SOURCE_DIR}/src/api/components/vkshader.cpp"
    "${CMAKE_SOURCE_DIR}/src/api/components/vkpipeline.cpp"
    "${CMAKE_SOURCE_DIR}/src/utils/option.hpp"
    "${CMAKE_SOURCE_DIR}/src/utils/file.hpp"
)

target_compile_definitions(
    ${PROJECT_NAME} PRIVATE
    RESOURCES="${CMAKE_SOURCE_DIR}/src/resources/"
)

target_link_libraries(${PROJECT_NAME} PRIVATE glfw vulkan)
target_include_directories(${PROJECT_NAME} PRIVATE modules/ src/)
