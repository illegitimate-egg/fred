#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>

#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/trigonometric.hpp>

#define IMGUI_DEFINE_MATH_OPERATORS // ImGui
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>
#include <imgui/imgui.h>
#define VGIZMO_USES_GLM
#include <imGuIZMO.quat/imGuIZMOquat.h>
#include <clog/clog.h>

#include <SOIL2.h>
#include <assimp/DefaultLogger.hpp>
#include <assimp/Importer.hpp>
#include <assimp/Logger.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include "shader.h"

constexpr int WIDTH = 1366;
constexpr int HEIGHT = 768;

static void glfwErrorCallback(int e, const char *description) {
  clog_log(CLOG_LEVEL_ERROR, "GLFW Error %d: %s", e, description);
}

namespace fred {

static bool loadModel(const char *path, std::vector<unsigned short> &indices,
                      std::vector<glm::vec3> &vertices,
                      std::vector<glm::vec2> &uvs,
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

GLuint loadTexture(const char *path) {
  GLuint texture = SOIL_load_OGL_texture(
      path, SOIL_LOAD_AUTO,
      SOIL_CREATE_NEW_ID,
      SOIL_FLAG_MIPMAPS | SOIL_FLAG_NTSC_SAFE_RGB | SOIL_FLAG_COMPRESS_TO_DXT | SOIL_FLAG_INVERT_Y);
  if (texture == 0) {
    clog_log(CLOG_LEVEL_WARN, "Texture failed to load");
    return 0;
  }
  return texture;
}

class Model {
public:
    std::vector<unsigned short> indices;
    GLuint vertexBuffer;
    GLuint uvBuffer;
    GLuint normalBuffer;
    GLuint elementBuffer;

  Model(std::string modelPath) {
    std::vector<glm::vec3> indexed_vertices;
    std::vector<glm::vec2> indexed_uvs;
    std::vector<glm::vec3> indexed_normals;

    loadModel(modelPath.c_str(), indices, indexed_vertices, indexed_uvs, indexed_normals);

    glGenBuffers(1, &vertexBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, indexed_vertices.size() * sizeof(glm::vec3),
               &indexed_vertices[0], GL_STATIC_DRAW);

    glGenBuffers(1, &uvBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, uvBuffer);
    glBufferData(GL_ARRAY_BUFFER, indexed_uvs.size() * sizeof(glm::vec2),
               &indexed_uvs[0], GL_STATIC_DRAW);

    glGenBuffers(1, &normalBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, normalBuffer);
    glBufferData(GL_ARRAY_BUFFER, indexed_normals.size() * sizeof(glm::vec3),
               &indexed_normals[0], GL_STATIC_DRAW);

    glGenBuffers(1, &elementBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, elementBuffer);
    glBufferData(GL_ARRAY_BUFFER, indices.size() * sizeof(unsigned short),
               &indices[0], GL_STATIC_DRAW);
  }
  ~Model() {
    // glDeleteBuffers(4, vertexBuffer); // Chances that these are contiguous in memory is zero
    glDeleteBuffers(1, &vertexBuffer);
    glDeleteBuffers(1, &uvBuffer);
    glDeleteBuffers(1, &normalBuffer);
    glDeleteBuffers(1, &elementBuffer);
  }
};

class Texture {
public:
  GLuint texture;

  Texture(std::string texturePath) {
    texture = loadTexture(texturePath.c_str());
  }
  ~Texture() {
    glDeleteTextures(1, &texture);
  }
};

class Shader {
public:
  GLuint shaderProgram;

  Shader(std::string vertPath, std::string fragPath) {
    shaderProgram = loadShaders(vertPath.c_str(), fragPath.c_str());
  }
  ~Shader() {
    glDeleteProgram(shaderProgram);
  }
};

class Asset {
public:
  std::vector<unsigned short> *indices;
  GLuint *vertexBuffer;
  GLuint *uvBuffer;
  GLuint *normalBuffer;
  GLuint *elementBuffer;

  GLuint matrixID;
  GLuint textureID;

  glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f);
  glm::quat rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f); // https://en.wikipedia.org/wiki/Quaternion
  glm::vec3 scaling = glm::vec3(1.0f, 1.0f, 1.0f);

  GLuint *modelTexture;

  GLuint *shaderProgram;

  Asset(Model &model, Texture &texture, Shader &shader) {
    indices = &model.indices;
    vertexBuffer = &model.vertexBuffer;
    uvBuffer = &model.uvBuffer;
    normalBuffer = &model.normalBuffer;
    elementBuffer = &model.elementBuffer;

    modelTexture = &texture.texture;

    shaderProgram = &shader.shaderProgram;

    matrixID = glGetUniformLocation(*shaderProgram, "MVP");
    textureID = glGetUniformLocation(*shaderProgram, "textureSampler");
  }
};

class Scene {
public:
  std::vector<Asset*> assets;
  void addAsset(Asset *asset) {
    assets.push_back(asset);
  }
};

GLFWwindow *window;

static bool shouldExit() {
return glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS && glfwWindowShouldClose(window) == 0;
}

glm::mat4 mvp; // TODO: REMOVE THIS PLLAAAHSE
glm::mat4 viewMatrix;
glm::mat4 projectionMatrix;

static int initWindow() {
  glfwSetErrorCallback(glfwErrorCallback);

  if (!glfwInit()) {
    clog_log(CLOG_LEVEL_ERROR, "GLFW went shitty time (failed to init)");
    return 1;
  }

  glfwWindowHint(GLFW_SAMPLES, 4); // 4x Antialiasing
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // Loser MacOS is broken
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); // New GL

  window = glfwCreateWindow(WIDTH, HEIGHT, "Fred", NULL, NULL);
  if (window == NULL) {
    clog_log(CLOG_LEVEL_ERROR, "Failed to open window.");
    glfwTerminate();
    return 1;
  }

  glfwMakeContextCurrent(window);
  glfwSwapInterval(1); // V-Sync

  int version;
  if ((version = gladLoadGL(glfwGetProcAddress))) {
    clog_log(CLOG_LEVEL_DEBUG, "GL version: %d.%d", GLAD_VERSION_MAJOR(version),
             GLAD_VERSION_MINOR(version));
  } else {
    clog_log(CLOG_LEVEL_ERROR, "Failed to init GL!");
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


  // I love glm
  // Projection matrix, 45deg FOV, 4:3 Aspect Ratio, Planes: 0.1units ->
  // 100units
  projectionMatrix = glm::perspective(
      glm::radians(45.0f), (float)WIDTH / (float)HEIGHT, 0.1f, 100.0f);

  // Camera matrix
  viewMatrix =
      glm::lookAt(glm::vec3(4, 3, 3), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));

  glm::mat4 model = glm::mat4(1.0f);

  // Model view projection, the combination of the three vectors
  mvp = projectionMatrix * viewMatrix * model;

  glEnable(GL_DEPTH_TEST); // Turn on the Z-buffer
  glDepthFunc(GL_LESS);    // Accept only the closest fragments

  glEnable(GL_CULL_FACE); // Backface culling

  glClearColor(0.1f, 0.1f, 0.1f, 1.0f);

  GLuint vertexArrayID;
  glGenVertexArrays(1, &vertexArrayID);
  glBindVertexArray(vertexArrayID);

  // Define camera stuff
  //glm::vec3 position = {4, 3, 3};
  //float horizontalAngle = 3.14f;
  //float verticalAngle = -0.1f;
  //float initialFOV = 60.0f;

  //float speed = 3.0f;
  //float mouseSpeed = 0.005f;

  //double xpos, ypos;

  //glDeleteVertexArrays(1, &vertexArrayID);
  //ImGui_ImplOpenGL3_Shutdown();
  //ImGui_ImplGlfw_Shutdown();
  //ImGui::DestroyContext();

  //glfwDestroyWindow(window);
  //glfwTerminate();

  return 0;
}

//void imguiMat4Table(glm::mat4 matrix, const char *name) {
//  if (ImGui::BeginTable(name, 4)) {
//    for (int i = 0; i < 4; i++) {
//      for (int j = 0; j < 4; j++) {
//        ImGui::TableNextColumn();
//        ImGui::Text("%f", matrix[i][j]);
//      }
//    }
//    ImGui::EndTable();
//  }
//}

void render(Scene scene) {
  // Clear this mf
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();

  for (int i = 0; i < scene.assets.size(); i++) {
    Asset *currentAsset = scene.assets[i];

    glUseProgram(*currentAsset->shaderProgram);

    // Send mega sigma MVP to the vertex shader (transformations for the win)
    glm::mat4 rotationMatrix = mat4_cast(currentAsset->rotation);
    glm::mat4 translationMatrix = glm::translate(glm::mat4(1), currentAsset->position);
    glm::mat4 scalingMatrix = glm::scale(glm::mat4(1), currentAsset->scaling);
    glm::mat4 modelMatrix = translationMatrix * rotationMatrix * scalingMatrix;

    mvp = projectionMatrix * viewMatrix * modelMatrix;

    char assetName[] = "Asset: 00";
    sprintf(assetName, "Asset: %d", i);
    ImGui::Begin(assetName);
    ImGui::gizmo3D(assetName, currentAsset->rotation);
    ImGui::End();

    glUniformMatrix4fv(currentAsset->matrixID, 1, GL_FALSE, &mvp[0][0]);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, *currentAsset->modelTexture);
    // Set sampler texture
    glUniform1i(currentAsset->textureID, 0);

    // DRAWING HAPPENS HERE
    // Vertex Data
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, *currentAsset->vertexBuffer);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void *)0);

    // UV Data
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, *currentAsset->uvBuffer);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (void *)0);

    // Normal Data
    glEnableVertexAttribArray(2);
    glBindBuffer(GL_ARRAY_BUFFER, *currentAsset->normalBuffer);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, (void *)0);

    // Index buffer
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, *currentAsset->elementBuffer);

    glDrawElements(GL_TRIANGLES, currentAsset->indices->size(), GL_UNSIGNED_SHORT,
                   (void *)0);

    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
    glDisableVertexAttribArray(2);
  }

  ImGui::Render();
  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

  // Swap buffers
  glfwSwapBuffers(window);
  glfwPollEvents();
}

} // namespace fred

int main() {
  fred::initWindow();
  fred::Model coneModel("../models/model.obj");
  fred::Texture buffBlackGuy("../textures/results/texture_BMP_DXT5_3.DDS");
  fred::Shader basicShader("../shaders/shader.vert", "../shaders/shader.frag");
  fred::Asset cone(coneModel, buffBlackGuy, basicShader);
  fred::Asset coneTwo(coneModel, buffBlackGuy, basicShader);

  fred::Scene scene = fred::Scene();

  scene.addAsset(&cone);
  scene.addAsset(&coneTwo);

  while (fred::shouldExit()) {
  //while (1) {
    fred::render(scene);
    cone.position.x += 0.01;
    glm::vec3 eulerAngles = glm::eulerAngles(coneTwo.rotation);
    eulerAngles.x += glm::radians(1.0f);
    coneTwo.rotation = glm::quat(eulerAngles);
  }

  return 0;
}
