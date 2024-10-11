cmake_minimum_required(VERSION 3.11)
project(fred C CXX)

# if($<CONFIG:Debug>)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON) # For my Clangd LSP
# endif()

set(GLFW_BUILD_WAYLAND OFF) # This should be detected and not forced
set(BUILD_SHARED_LIBS OFF) # Keep the project as one binary (glew, glm)
set(GLM__BUILD_TESTS OFF) # Don't build GLM tests
set(GLFW_BUILD_EXAMPLES OFF) # Don't build GLFW Examples
set(GLFW_BUILD_TESTS OFF) # Don't build GLFW Tests
set(GLFW_INSTALL OFF) # We're not building a standalone
# set(GLEW_BUILD_DOCS $<CONFIG:Debug>) # Build docs if debug
set(SOIL2_BUILD_TESTS OFF) # Don't build SOIL2 Tests
set(ASSIMP_BUILD_ASSIMP_TOOLS OFF)
set(ASSIMP_BUILD_TESTS OFF)

find_package(OpenGL REQUIRED)

if(CMAKE_BINARY_DIR STREQUAL CMAKE_SOURCE_DIR)
  message(FATAL_ERROR "You fucking smell fr\n")
endif()
if(CMAKE_SOURCE_DIR MATCHES " ")
  message(
    WARNING "Spaces in the source dir can cause errors, thou art been warned\n")
endif()
if(CMAKE_BINARY_DIR MATCHES " ")
  message(
    WARNING "Spaces in the build dir can cause errors, thou art been warned\n")
endif()

add_subdirectory(include/glm)
add_subdirectory(include/glfw)
add_subdirectory(include/SOIL2)
add_subdirectory(include/assimp)
add_subdirectory(include/CLog)

# Glad is configured using cmake (unlike glew)
# https://github.com/Dav1dde/glad/wiki/C#cmake
set(GLAD_SOURCES_DIR "${PROJECT_SOURCE_DIR}/include/glad")
add_subdirectory("${GLAD_SOURCES_DIR}/cmake" glad_cmake)

glad_add_library(glad_gl_core_33 REPRODUCIBLE API gl:core=3.3)

include_directories(include/imgui) # ImGui doesn't have a CMakeLists of its own
add_library(
  imgui STATIC
  include/imgui/imgui.cpp
  include/imgui/imgui_demo.cpp
  include/imgui/imgui_draw.cpp
  include/imgui/imgui_tables.cpp
  include/imgui/imgui_widgets.cpp
  include/imgui/backends/imgui_impl_glfw.cpp # Zingaloid backend import
  include/imgui/backends/imgui_impl_opengl3.cpp)
target_link_libraries(imgui glfw)

include_directories(include/ImGuizmo)
add_library(
  imGuizmo STATIC
  include/ImGuizmo/GraphEditor.cpp
  include/ImGuizmo/ImCurveEdit.cpp
  include/ImGuizmo/ImGradient.cpp
  include/ImGuizmo/ImGuizmo.cpp
  include/ImGuizmo/ImSequencer.cpp
)
target_link_libraries(imGuizmo imgui)

add_executable(fred engine.cpp shader.c)
target_link_libraries(fred $<$<PLATFORM_ID:Linux>:-lm> glad_gl_core_33 glm glfw
                      soil2 assimp imgui imGuizmo clog)
