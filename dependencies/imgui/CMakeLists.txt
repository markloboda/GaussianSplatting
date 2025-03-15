cmake_minimum_required(VERSION 3.15)
project(imgui LANGUAGES CXX)

# Set C++ standard (ImGui works with C++11 and later)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# List ImGui core source files
set(IMGUI_CORE
    imgui.cpp
    imgui_draw.cpp
    imgui_tables.cpp
    imgui_widgets.cpp
    imgui_demo.cpp
)

# List backend sources (adjust or remove backends you don't need)
set(IMGUI_BACKENDS
    backends/imgui_impl_glfw.cpp
    backends/imgui_impl_wgpu.cpp
)

# Define the backend to WGPU
add_compile_definitions(IMGUI_IMPL_WEBGPU_BACKEND_WGPU)

# Create a static library called 'imgui'
add_library(imgui STATIC ${IMGUI_CORE} ${IMGUI_BACKENDS})

# Include the ImGui root and backends directories so that the headers can be found
target_include_directories(imgui PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR} 
    ${CMAKE_CURRENT_SOURCE_DIR}/backends
)

target_link_libraries(imgui PRIVATE glfw webgpu)
