#version 330 core

layout(location = 0) in vec3 vertexPosition_modelspace;
layout(location = 1) in vec2 vertexUV;
layout(location = 2) in vec3 vertexNormal_modelspace;

out vec2 UV;
out vec3 position_worldspace;
out vec3 normal_cameraspace;
out vec3 eyeDirection_cameraspace;
out vec3 lightDirection_cameraspace;

uniform mat4 mvp;
uniform mat4 v;
uniform mat4 m;
uniform vec3 lightPosition_worldspace;

void main() {
    gl_Position = mvp * vec4(vertexPosition_modelspace, 1);

    position_worldspace = (m * vec4(vertexPosition_modelspace, 1)).xyz;

    vec3 vertexPosition_cameraspace = (v * m * vec4(vertexPosition_modelspace, 1)).xyz;
    eyeDirection_cameraspace = vec3(0, 0, 0) - vertexPosition_cameraspace;

    vec3 lightPosition_cameraspace = (v * vec4(lightPosition_worldspace, 1)).xyz;
    lightDirection_cameraspace = lightPosition_cameraspace + eyeDirection_cameraspace;

    normal_cameraspace = (v * m * vec4(vertexNormal_modelspace, 0)).xyz;

    UV = vertexUV;
}
