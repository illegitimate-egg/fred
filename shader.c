#include <stdio.h>
#include <stdlib.h>

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#ifdef __linux__
#include <unistd.h>
#endif
#ifdef _WIN32
#include <io.h>
#define lseek _lseek
#endif

#include <clog/clog.h>

GLuint loadShaders(const char *vertex_file_path,
                   const char *fragment_file_path) {
  GLuint vertexShaderID = glCreateShader(GL_VERTEX_SHADER);
  GLuint fragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);

  FILE *vertexShaderFD;
  vertexShaderFD = fopen(vertex_file_path, "rb");
  if (vertexShaderFD == NULL) {
    fclose(vertexShaderFD);
    clog_log(CLOG_LEVEL_ERROR, "Failed to open Vertex Shader");
    return 1;
  }
  int vertexShaderLength = lseek(fileno(vertexShaderFD), 0L, SEEK_END) + 1;
  fseek(vertexShaderFD, 0L, SEEK_SET);
  char *vertexShaderCode = (char *)calloc(vertexShaderLength, sizeof(char));
  fread(vertexShaderCode, sizeof(*vertexShaderCode), vertexShaderLength,
        vertexShaderFD);
  fclose(vertexShaderFD);

  FILE *fragmentShaderFD;
  fragmentShaderFD = fopen(fragment_file_path, "rb");
  if (fragmentShaderFD == NULL) {
    fclose(fragmentShaderFD);
    clog_log(CLOG_LEVEL_ERROR, "Failed to open Fragment Shader");
    return 1;
  }
  int fragmentShaderLength = lseek(fileno(fragmentShaderFD), 0L, SEEK_END) + 1;
  fseek(fragmentShaderFD, 0L, SEEK_SET);
  char *fragmentShaderCode = (char *)calloc(fragmentShaderLength, sizeof(char));
  fread(fragmentShaderCode, sizeof(*fragmentShaderCode), fragmentShaderLength,
        fragmentShaderFD);
  fclose(fragmentShaderFD);

  GLint result = GL_FALSE;
  int infoLogLength;

  clog_log(CLOG_LEVEL_DEBUG, "Compiling shader: %s", vertex_file_path);
  char const *vertexShaderCodeConst = vertexShaderCode;
  glShaderSource(vertexShaderID, 1, &vertexShaderCodeConst, NULL);
  glCompileShader(vertexShaderID);

  glGetShaderiv(vertexShaderID, GL_COMPILE_STATUS, &result);
  glGetShaderiv(vertexShaderID, GL_INFO_LOG_LENGTH, &infoLogLength);
  if (infoLogLength > 0) {
    char *vertexShaderErrorMessage[infoLogLength + 1];
    glGetShaderInfoLog(vertexShaderID, infoLogLength, NULL,
                       *vertexShaderErrorMessage);
    clog_log(CLOG_LEVEL_ERROR, "%s", *vertexShaderErrorMessage);
  }

  clog_log(CLOG_LEVEL_DEBUG, "Compiling shader: %s", fragment_file_path);
  char const *fragmentShaderCodeConst = fragmentShaderCode;
  glShaderSource(fragmentShaderID, 1, &fragmentShaderCodeConst, NULL);
  glCompileShader(fragmentShaderID);

  glGetShaderiv(fragmentShaderID, GL_COMPILE_STATUS, &result);
  glGetShaderiv(fragmentShaderID, GL_INFO_LOG_LENGTH, &infoLogLength);
  if (infoLogLength > 0) {
    char *fragmentShaderErrorMessage[infoLogLength + 1];
    glGetShaderInfoLog(fragmentShaderID, infoLogLength, NULL,
                       *fragmentShaderErrorMessage);
    clog_log(CLOG_LEVEL_ERROR, "%s", *fragmentShaderErrorMessage);
  }

  clog_log(CLOG_LEVEL_DEBUG, "Linking shader program");
  GLuint programID = glCreateProgram();
  glAttachShader(programID, vertexShaderID);
  glAttachShader(programID, fragmentShaderID);
  glLinkProgram(programID);

  glGetProgramiv(programID, GL_COMPILE_STATUS, &result);
  glGetProgramiv(programID, GL_INFO_LOG_LENGTH, &infoLogLength);
  if (infoLogLength > 0) {
    char *programErrorMessage[infoLogLength + 1];
    glGetProgramInfoLog(programID, infoLogLength, NULL, *programErrorMessage);
    clog_log(CLOG_LEVEL_ERROR, "%s", *programErrorMessage);
  }

  glDetachShader(programID, vertexShaderID);
  glDetachShader(programID, fragmentShaderID);

  glDeleteShader(vertexShaderID);
  glDeleteShader(fragmentShaderID);

  free(vertexShaderCode);
  free(fragmentShaderCode);

  return programID;
}
