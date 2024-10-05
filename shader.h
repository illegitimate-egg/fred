#ifndef LOAD_SHADERS_H
#define LOAD_SHADERS_H

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <unistd.h>

GLuint loadShaders(const char *vertex_file_path,
                   const char *fragment_file_path);

#endif
