cmake_minimum_required(VERSION 3.11)
project(fred)

# if($<CONFIG:Debug>)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON) # For my Clangd LSP
# endif()

set(GLFW_BUILD_WAYLAND OFF) # This should be detected and not forced

find_package(OpenGL REQUIRED)

if(CMAKE_BINARY_DIR STREQUAL CMAKE_SOURCE_DIR)
  message(FATAL_ERROR "You fucking smell fr")
endif()
if(CMAKE_SOURCE_DIR MATCHES " ")
  message("Spaces in the source dir can cause errors, thou art been warned")
endif()
if(CMAKE_BIANRY_DIR MATCHES " ")
  message("Spaces in the build dir can cause errors, thou art been warned")
endif()

add_custom_command(
  OUTPUT ${PROJECT_SOURCE_DIR}/include/glew/src/glew.c
         ${PROJECT_SOURCE_DIR}/include/glewinfo.c
  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/include/glew/auto
  COMMAND ${CMAKE_MAKE_PROGRAM})

add_subdirectory(include/glm)
add_subdirectory(include/glew/build/cmake)
add_subdirectory(include/glfw)
add_subdirectory(include/SOIL2)
add_subdirectory(include/assimp)

include_directories(include/imgui)
add_library(
  imgui
  include/imgui/imgui.cpp
  include/imgui/imgui_demo.cpp
  include/imgui/imgui_draw.cpp
  include/imgui/imgui_tables.cpp
  include/imgui/imgui_widgets.cpp
  include/imgui/backends/imgui_impl_glfw.cpp # Zingaloid backend import
  include/imgui/backends/imgui_impl_opengl3.cpp)
target_link_libraries(imgui glfw)

add_executable(fred engine.cpp shader.c)
target_link_libraries(
  fred
  -lm
  glm
  glew
  glfw
  soil2
  assimp
  imgui)
