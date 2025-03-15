cmake_minimum_required(VERSION 3.20)

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project(GaussianSplatting)

set(CMAKE_CXX_STANDARD 26)

# Define path variables
set(REPO_ROOT ${CMAKE_CURRENT_SOURCE_DIR})
set(SRC_ROOT ${REPO_ROOT}/src)
set(SHADERS_ROOT ${REPO_ROOT}/assets/shaders)
set(DEPENDENCIES_ROOT ${REPO_ROOT}/dependencies)

# Source files
set(SOURCE_FILES
        ${SRC_ROOT}/main.cpp
        ${SRC_ROOT}/Application/Application.cpp
        ${SRC_ROOT}/Application/Application.h
        ${SRC_ROOT}/GaussianSplatting.h
        ${SRC_ROOT}/Utils/FileReader.cpp
        ${SRC_ROOT}/Utils/FileReader.h
        ${SRC_ROOT}/Core/Core.h
        ${SRC_ROOT}/Application/Renderer.cpp
        ${SRC_ROOT}/Application/Renderer.h
        ${SRC_ROOT}/Application/Camera.h
        ${SRC_ROOT}/Application/InputManager.cpp
        ${SRC_ROOT}/Application/InputManager.h
        ${SRC_ROOT}/Application/Camera.cpp
)

# Group source files
source_group(TREE ${REPO_ROOT} FILES ${SOURCE_FILES})

# Shader files
set(SHADER_FILES
    ${SHADERS_ROOT}/gaussian_splatting.wgsl
)

# Group shader files
source_group(TREE ${REPO_ROOT} FILES ${SHADER_FILES})

# Include library directories
add_subdirectory(${DEPENDENCIES_ROOT}/glfw)
add_subdirectory(${DEPENDENCIES_ROOT}/glfw3webgpu)
add_subdirectory(${DEPENDENCIES_ROOT}/glm)
add_subdirectory(${DEPENDENCIES_ROOT}/imgui)
add_subdirectory(${DEPENDENCIES_ROOT}/webgpu)

# Add executable
add_executable(${PROJECT_NAME} ${SOURCE_FILES} ${SHADER_FILES})
target_include_directories(${PROJECT_NAME} PRIVATE ${SRC_ROOT})
target_link_libraries(${PROJECT_NAME} PRIVATE glfw webgpu glfw3webgpu glm imgui)

# Copy webgpu binaries
target_copy_webgpu_binaries(${PROJECT_NAME})