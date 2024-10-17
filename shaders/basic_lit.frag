#version 330 core

in vec2 UV;
in vec3 position_worldspace;
in vec3 normal_cameraspace;
in vec3 eyeDirection_cameraspace;
in vec3 lightDirection_cameraspace;

layout(location = 0) out vec3 color;

uniform sampler2D albedoSampler;
uniform sampler2D specularSampler;
uniform mat4 mv; // mv of mvp (Model, View, Projection)
uniform vec3 lightPosition_worldspace;
uniform vec3 lightColor;
uniform float lightPower;

void main() {
  vec3 materialDiffuseColor = texture(albedoSampler, UV).rgb;
  vec3 materialAmbientColor = vec3(0.1, 0.1, 0.1) * materialDiffuseColor;
  vec3 materialSpecularColor = texture(specularSampler, UV).rgb;

  float distance = length(lightPosition_worldspace - position_worldspace);

  vec3 n = normalize(normal_cameraspace);
  vec3 l = normalize(lightDirection_cameraspace);
  float cosTheta = clamp(dot(n, vec3(1.0)), 0, 1);

  vec3 E = normalize(eyeDirection_cameraspace);
  vec3 R = reflect(vec3(-1.0), n);
  float cosAlpha = clamp(dot(E, R), 0, 1);

  color = materialAmbientColor + materialDiffuseColor * lightColor * lightPower * cosTheta / (distance*distance) + materialSpecularColor * lightColor * lightPower * pow(cosAlpha, 5) / (distance * distance);
}
