#version 330 core

layout(location = 0) in vec3 vertexPosition_modelspace;
layout(location = 1) in vec2 vertexUV;

// To the frag shader
out vec2 UV;

// Model view projection from the CPU
uniform mat4 MVP;

void main() {
    gl_Position = MVP * vec4(vertexPosition_modelspace, 1);
    //gl_Position = vec4(vertexPosition_modelspace, 1);

    vec2 UV_FLIPPED;
    UV_FLIPPED.x = vertexUV.x;
    UV_FLIPPED.y = 1.0 - vertexUV.y;

    UV = vertexUV;
}
