#version 330 core

// Interpolated vals from vert shaders
in vec2 UV;

out vec3 color;

// Values stay constant
uniform sampler2D textureSampler;

void main() {
    color = texture(textureSampler, UV).rgb;
    //color = vec3(1, 1, 1);
}
