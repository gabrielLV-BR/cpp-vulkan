#version 450

layout(location = 0) out vec4 outColor;
layout(location = 0) in vec3 aColor;

void main() {
  outColor = vec4(aColor, 1.0);
}