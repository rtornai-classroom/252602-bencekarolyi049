#version 330 core

layout (location = 0) in vec2 aPos;
uniform mat4 u_projection;
uniform float u_pointSize; // A pontok átmérője

void main() {
    gl_Position = u_projection * vec4(aPos, 0.0, 1.0);
    gl_PointSize = u_pointSize; // Beállítjuk a kért pötty-méretet
}