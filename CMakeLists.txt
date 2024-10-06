cmake_minimum_required(VERSION 3.11)
project(fred)

# if($<CONFIG:Debug>)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON) # For my Clangd LSP
# endif()

set(GLFW_BUILD_WAYLAND OFF)  # This should be detected and not forced
set(BUILD_SHARED_LIBS OFF)   # Keep the project as one binary (glew, glm)
set(GLM__BUILD_TESTS OFF)    # Don't build GLM tests
set(GLFW_BUILD_EXAMPLES OFF) # Don't build GLFW Examples
set(GLFW_BUILD_TESTS OFF)    # Don't build GLFW Tests
set(GLFW_INSTALL OFF)        # We're not building a standalone
# set(GLEW_BUILD_DOCS $<CONFIG:Debug>) # Build docs if debug
set(SOIL2_BUILD_TESTS OFF)   # Don't build SOIL2 Tests


find_package(OpenGL REQUIRED)

if(CMAKE_BINARY_DIR STREQUAL CMAKE_SOURCE_DIR)
  message(FATAL_ERROR "You fucking smell fr\n")
endif()
if(CMAKE_SOURCE_DIR MATCHES " ")
  message(WARNING "Spaces in the source dir can cause errors, thou art been warned\n")
endif()
if(CMAKE_BINARY_DIR MATCHES " ")
  message(WARNING "Spaces in the build dir can cause errors, thou art been warned\n")
endif()

if(UNIX)
  message(STATUS "Configuring GLEW for *nix (Non-cmake)\n")
  add_custom_command(
    OUTPUT ${PROJECT_SOURCE_DIR}/include/glew/src/glew.c
           ${PROJECT_SOURCE_DIR}/include/glewinfo.c
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/include/glew/auto
    COMMAND ${CMAKE_MAKE_PROGRAM})
	
	add_subdirectory(include/glew/build/cmake)
	
	if(NOT (EXISTS "${PROJECT_SOURCE_DIR}/include/glew/src/glew.c" AND EXISTS "${PROJECT_SOURCE_DIR}/include/glewinfo.c"))
        message(FATAL_ERROR "GLEW failed to configure!")
    endif()
endif()
if(WIN32)
  message(STATUS "Download GLEW for Windows (Non-cmake)\n") # I wanted to make this work from source, but the auto config is make only, and requiring and/or shipping msys2 would be unreasonable so this is the best compromise
  file(DOWNLOAD https://github.com/nigels-com/glew/releases/download/glew-2.2.0/glew-2.2.0-win32.zip ${CMAKE_BINARY_DIR}/glew-2.2.0.zip)
  execute_process(COMMAND tar -xf ${CMAKE_BINARY_DIR}/glew-2.2.0.zip # bdstar
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
	
  if(NOT EXISTS ${CMAKE_BINARY_DIR}/glew-2.2.0/include/GL/glew.h)
	message(FATAL_ERROR "GLEW Failed to download/decompress")
  endif()
	
  include_directories(${CMAKE_BINARY_DIR}/glew-2.2.0/include)
  add_library(glew STATIC # There aren't actually any binaries in here, so no lib is to be built
    ${CMAKE_BINARY_DIR}/glew-2.2.0/include/GL/eglew.h
    ${CMAKE_BINARY_DIR}/glew-2.2.0/include/GL/glew.h
    ${CMAKE_BINARY_DIR}/glew-2.2.0/include/GL/glxew.h
    ${CMAKE_BINARY_DIR}/glew-2.2.0/include/GL/wglew.h)
  set_target_properties(glew PROPERTIES LINKER_LANGUAGE C)
  if(CMAKE_SIZEOF_VOID_P EQUAL 8) # 64-bit
    target_link_libraries(glew ${CMAKE_BINARY_DIR}/glew-2.2.0/lib/Release/x64/glew32.lib ${CMAKE_BINARY_DIR}/glew-2.2.0/lib/Release/x64/glew32s.lib)
  elseif(CMAKE_SIZEOF_VOID_P EQUAL 4) # 32-bit, if it's not either of these you're in trouble
    target_link_libraries(glew ${CMAKE_BINARY_DIR}/glew-2.2.0/lib/Release/Win32/glew32.lib ${CMAKE_BINARY_DIR}/glew-2.2.0/lib/Release/Win32/glew32s.lib)
  endif()
endif()

add_subdirectory(include/glm)
add_subdirectory(include/glfw)
add_subdirectory(include/SOIL2)
add_subdirectory(include/assimp)
add_subdirectory(include/CLog)

include_directories(include/imgui)
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

add_executable(fred engine.cpp shader.c)
if (UNIX)
target_link_libraries(
  fred
  -lm
  glew
  glm
  glfw
  soil2
  assimp
  imgui
  clog)
 endif()
 if (WIN32)
 target_link_libraries(
  fred
  glew
  glm
  glfw
  soil2
  assimp
  imgui
  clog)
 endif()