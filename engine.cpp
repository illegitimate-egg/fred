#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/trigonometric.hpp>

#include <clog/clog.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>
#include <imgui.h>

#include <SOIL2.h>
#include <assimp/DefaultLogger.hpp>
#include <assimp/Importer.hpp>
#include <assimp/Logger.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include "shader.h"

constexpr int WIDTH = 1024;
constexpr int HEIGHT = 768;

static void glfwErrorCallback(int e, const char *description) {
  clog_log(CLOG_LEVEL_ERROR, "GLFW Error %d: %s", e, description);
}

static bool loadModel(const char *path, std::vector<unsigned short> &indices,
               std::vector<glm::vec3> &vertices, std::vector<glm::vec2> &uvs,
               std::vector<glm::vec3> &normals) {
  Assimp::Importer importer;

  const aiScene *scene = importer.ReadFile(
      path, aiProcess_Triangulate | aiProcess_JoinIdenticalVertices |
                aiProcess_SortByPType);
  if (!scene) {
    clog_log(CLOG_LEVEL_ERROR, "%s", importer.GetErrorString());
    return false;
  }
  const aiMesh *mesh = scene->mMeshes[0];

  vertices.reserve(mesh->mNumVertices);
  for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
    aiVector3D pos = mesh->mVertices[i];
    vertices.push_back(glm::vec3(pos.x, pos.y, pos.z));
  }

  uvs.reserve(mesh->mNumVertices);
  for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
    aiVector3D UVW = mesh->mTextureCoords[0][i]; // Multiple UVs? Prepsterous!
    uvs.push_back(glm::vec2(UVW.x, UVW.y));
  }

  normals.reserve(mesh->mNumVertices);
  for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
    indices.push_back(mesh->mFaces[i].mIndices[0]);
    indices.push_back(mesh->mFaces[i].mIndices[1]);
    indices.push_back(mesh->mFaces[i].mIndices[2]);
  }

  return true;
}

static int initWindow() {
  glfwSetErrorCallback(glfwErrorCallback);

  glewExperimental = true;
  if (!glfwInit()) {
    clog_log(CLOG_LEVEL_ERROR, "GLFW went shitty time (failed to init)");
    return 1;
  }

  glfwWindowHint(GLFW_SAMPLES, 4); // 4x Antialiasing
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // Loser MacOS is broken
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); // New GL

  GLFWwindow *window;
  window = glfwCreateWindow(WIDTH, HEIGHT, "Fred", NULL, NULL);
  if (window == NULL) {
    clog_log(CLOG_LEVEL_ERROR, "Failed to open window.");
    glfwTerminate();
    return 1;
  }

  glfwMakeContextCurrent(window);
  glfwSwapInterval(1);
  int code;
  if ((code = glewInit()) != GLEW_OK) {
    clog_log(CLOG_LEVEL_ERROR, "Failed to init GLEW: %s", glewGetErrorString(code));
    return 1;
  }

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  (void)io;
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

  ImGui::StyleColorsDark();

  ImGui_ImplGlfw_InitForOpenGL(window, true);
  ImGui_ImplOpenGL3_Init("#version 330");

  std::vector<unsigned short> indices;
  std::vector<glm::vec3> indexed_vertices;
  std::vector<glm::vec2> indexed_uvs;
  std::vector<glm::vec3> indexed_normals;
  if (!loadModel("../models/model.obj", indices, indexed_vertices, indexed_uvs,
                 indexed_normals)) {
    return 0;
  }

  GLuint vertexArrayID;
  glGenVertexArrays(1, &vertexArrayID);
  glBindVertexArray(vertexArrayID);

  GLuint vertexBuffer;
  glGenBuffers(1, &vertexBuffer);
  glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
  glBufferData(GL_ARRAY_BUFFER, indexed_vertices.size() * sizeof(glm::vec3),
               &indexed_vertices[0], GL_STATIC_DRAW);

  GLuint uvBuffer;
  glGenBuffers(1, &uvBuffer);
  glBindBuffer(GL_ARRAY_BUFFER, uvBuffer);
  glBufferData(GL_ARRAY_BUFFER, indexed_uvs.size() * sizeof(glm::vec2),
               &indexed_uvs[0], GL_STATIC_DRAW);

  GLuint normalBuffer;
  glGenBuffers(1, &normalBuffer);
  glBindBuffer(GL_ARRAY_BUFFER, normalBuffer);
  glBufferData(GL_ARRAY_BUFFER, indexed_normals.size() * sizeof(glm::vec3),
               &indexed_normals[0], GL_STATIC_DRAW);

  GLuint elementBuffer;
  glGenBuffers(1, &elementBuffer);
  glBindBuffer(GL_ARRAY_BUFFER, elementBuffer);
  glBufferData(GL_ARRAY_BUFFER, indices.size() * sizeof(unsigned short),
               &indices[0], GL_STATIC_DRAW);

  GLuint texture = SOIL_load_OGL_texture(
      "../textures/results/texture_BMP_DXT5_3.DDS", SOIL_LOAD_AUTO,
      SOIL_CREATE_NEW_ID,
      SOIL_FLAG_MIPMAPS | SOIL_FLAG_NTSC_SAFE_RGB | SOIL_FLAG_COMPRESS_TO_DXT);
  if (texture == 0) {
    clog_log(CLOG_LEVEL_WARN, "Texture failed to load");
    return 0;
  }

  GLuint programID =
      loadShaders("../shaders/shader.vert", "../shaders/shader.frag");

  // I love glm
  // Projection matrix, 45deg FOV, 4:3 Aspect Ratio, Planes: 0.1units ->
  // 100units
  glm::mat4 projection = glm::perspective(
      glm::radians(45.0f), (float)WIDTH / (float)HEIGHT, 0.1f, 100.0f);

  // Camera matrix
  glm::mat4 view =
      glm::lookAt(glm::vec3(4, 3, 3), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));

  glm::mat4 model = glm::mat4(1.0f);

  // Model view projection, the combination of the three vectors
  glm::mat4 mvp = projection * view * model;

  GLuint matrixID = glGetUniformLocation(programID, "MVP");

  glEnable(GL_DEPTH_TEST); // Turn on the Z-buffer
  glDepthFunc(GL_LESS);    // Accept only the closest fragments

  glEnable(GL_CULL_FACE); // Backface culling

  glClearColor(0.1f, 0.1f, 0.1f, 1.0f);

  // Define camera stuff
  glm::vec3 position = {4, 3, 3};
  float horizontalAngle = 3.14F; // Toward -Z, in Rads
  float verticalAngle = 0.0f;    // On the horizon
  float initialFOV = 60.0f;

  float speed = 3.0f;
  float mouseSpeed = 0.005f;

  double xpos, ypos;

  double lastTime = 0;
  while (glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS &&
         glfwWindowShouldClose(window) == 0) {
    // Clear this mf
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glUseProgram(programID);

    // ImGui
    // if (glfwGetWindowAttrib(window, GLFW_ICONIFIED) != 0) {
    //   ImGui_ImplGlfw_Sleep(10);
    //   continue; // No rendering while minimized
    // }

    glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();

    ImGui::NewFrame();

    // ImGui::ShowDemoWindow();
    ImGui::Begin("DEBUG INFO");
    ImGui::Text("MVP:");
    if (ImGui::BeginTable("mvp", 4)) {
      ImGui::TableNextColumn();
      ImGui::Text("%f", mvp[0][0]);
      ImGui::TableNextColumn();
      ImGui::Text("%f", mvp[0][1]);
      ImGui::TableNextColumn();
      ImGui::Text("%f", mvp[0][2]);
      ImGui::TableNextColumn();
      ImGui::Text("%f", mvp[0][3]);
      ImGui::TableNextColumn();
      ImGui::Text("%f", mvp[1][0]);
      ImGui::TableNextColumn();
      ImGui::Text("%f", mvp[1][1]);
      ImGui::TableNextColumn();
      ImGui::Text("%f", mvp[1][2]);
      ImGui::TableNextColumn();
      ImGui::Text("%f", mvp[1][3]);
      ImGui::TableNextColumn();
      ImGui::Text("%f", mvp[2][0]);
      ImGui::TableNextColumn();
      ImGui::Text("%f", mvp[2][1]);
      ImGui::TableNextColumn();
      ImGui::Text("%f", mvp[2][2]);
      ImGui::TableNextColumn();
      ImGui::Text("%f", mvp[2][3]);
      ImGui::TableNextColumn();
      ImGui::Text("%f", mvp[3][0]);
      ImGui::TableNextColumn();
      ImGui::Text("%f", mvp[3][1]);
      ImGui::TableNextColumn();
      ImGui::Text("%f", mvp[3][2]);
      ImGui::TableNextColumn();
      ImGui::Text("%f", mvp[3][3]);

      ImGui::EndTable();
    }
    ImGui::End();

    // Compute a Fresh Super Sigma MVP
    // What would an input system be without delta time
    double currentTime = glfwGetTime();
    float deltaTime = (float)(currentTime - lastTime);

    glfwGetCursorPos(window, &xpos, &ypos);
    glfwSetCursorPos(window, (float)(WIDTH) / 2, (float)(HEIGHT) / 2);

    horizontalAngle +=
        mouseSpeed * deltaTime * (float)((float)(WIDTH) / 2 - xpos);
    verticalAngle +=
        mouseSpeed * deltaTime * (float)((float)(HEIGHT) / 2 - ypos);
    glm::vec3 direction(cos(verticalAngle) * sin(horizontalAngle),
                        sin(verticalAngle),
                        cos(verticalAngle) * cos(horizontalAngle));
    glm::vec3 right(sin(horizontalAngle - 3.14f / 2.0f), 0,
                    cos(horizontalAngle - 3.14f / 2.0f));
    glm::vec3 up = glm::cross(right, direction);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
      position += direction * deltaTime * speed;
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
      position -= direction * deltaTime * speed;
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
      position += right * deltaTime * speed;
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
      position -= right * deltaTime * speed;
    }

    // float FOV = initialFOV;
    // Disabled because unchanged
    // glm_perspective(glm_rad(FOV), 4.0f/ 3.0f, 0.1f, 100.0f, projection);
    glm::mat4 view = glm::lookAt(position, position + direction, up);
    mvp = projection * view * model;

    // Send mega sigma MVP to the vertex shader (transformations for the win)
    glUniformMatrix4fv(matrixID, 1, GL_FALSE, &mvp[0][0]);

    // DRAWING HAPPENS HERE
    // Vertex Data
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void *)0);

    // UV Data
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, uvBuffer);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (void *)0);

    // Normal Data
    glEnableVertexAttribArray(2);
    glBindBuffer(GL_ARRAY_BUFFER, normalBuffer);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, (void *)0);

    // Index buffer
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementBuffer);

    glDrawElements(GL_TRIANGLES, (GLsizei)indices.size(), GL_UNSIGNED_SHORT, (void *)0);

    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
    glDisableVertexAttribArray(2);

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    // Swap buffers
    glfwSwapBuffers(window);
    glfwPollEvents();

    lastTime = currentTime; // Count total frame time
  }

  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();

  glDeleteBuffers(1, &vertexBuffer);
  glDeleteBuffers(1, &uvBuffer);
  glDeleteBuffers(1, &normalBuffer);
  glDeleteBuffers(1, &elementBuffer);
  glDeleteProgram(programID);
  glDeleteTextures(1, &texture);
  glDeleteVertexArrays(1, &vertexArrayID);

  glfwDestroyWindow(window);
  glfwTerminate();

  return 0;
}

int main() { return initWindow(); }
